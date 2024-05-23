// Simple PIO-based (non-DMA) IDE driver code.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

#define SECTOR_SIZE   512
#define IDE_BSY       0x80
#define IDE_DRDY      0x40
#define IDE_DF        0x20
#define IDE_ERR       0x01

#define IDE_CMD_READ  0x20
#define IDE_CMD_WRITE 0x30
#define IDE_CMD_READ2  0x21
#define IDE_CMD_WRITE2 0x31
#define IDE_CMD_RDMUL 0xc4
#define IDE_CMD_WRMUL 0xc5

// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.
// You must hold idelock while manipulating queue.

static struct spinlock idelock;
static struct spinlock idelock2;
static struct buf *idequeue;
static struct buf *idequeue2;


static int havedisk1;
static int havedisk2;

static void idestart(uint dev, struct buf *);

// Wait for IDE disk to become ready.
static int
idewait(int dev, int checkerr) {
    int r;
    int base_port = (dev == 1) ? 0x1f7 : 0x177; // Base port for disk 0 or disk 1

    if (dev == 2)
        base_port = 0x177; // Base port for disk 2

    while (((r = inb(base_port)) & (IDE_BSY | IDE_DRDY)) != IDE_DRDY);
    if (checkerr && (r & (IDE_DF | IDE_ERR)) != 0)
        return -1;
    return 0;
}

void
ideinit(void) {
    int i;

    initlock(&idelock, "ide");
    initlock(&idelock2, "ide2");
    ioapicenable(IRQ_IDE, ncpu - 1);
    idewait(1,0);
   // idewait(2,0);

    // Check if disk 1 is present
    outb(0x1f6, 0xe0 | (1 << 4));
    for (i = 0; i < 1000; i++) {
        if (inb(0x1f7) != 0) {
            havedisk1 = 1;
            break;
        }
    }

    // Check if disk 2 is present
    outb(0x1f6, 0xe0 | (2 << 4)); // Select disk 2
    for (i = 0; i < 1000; i++) {
        if (inb(0x177) != 0) { // Check status register for disk 2
            havedisk2 = 1;
            break;
        }
    }

    // Switch back to disk 0.
    outb(0x1f6, 0xe0 | (0 << 4));
}

// Start the request for b.  Caller must hold idelock.
static void
idestart(uint dev, struct buf *b) {
    if (b == 0)
        panic("idestart");
    if (b->blockno >= FSSIZE)
        panic("incorrect blockno");
    int sector_per_block = BSIZE / SECTOR_SIZE;
    int sector = b->blockno * sector_per_block;
    int read_cmd, write_cmd;
    int base_port;
    // Select the appropriate command based on the disk
    if(dev == 1) {
        read_cmd = (sector_per_block == 1) ? IDE_CMD_READ :  IDE_CMD_RDMUL;
        write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;
        base_port = 0x177;
    } else {
        // Disk 2
        read_cmd = (sector_per_block == 1) ? IDE_CMD_READ2 :  IDE_CMD_RDMUL;
        write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE2 : IDE_CMD_WRMUL;
        base_port = 0x1f7;
    }

    if (sector_per_block > 7) panic("idestart");

    idewait(dev,0);
    outb(0x3f6, 0);  // generate interrupt
    outb(0x1f2, sector_per_block);  // number of sectors
    outb(0x1f3, sector & 0xff);
    outb(0x1f4, (sector >> 8) & 0xff);
    outb(0x1f5, (sector >> 16) & 0xff);
    outb(0x1f6, 0xe0 | ((dev & 0x01 ? 0x00 : 0x10) | ((dev & 0x02) ? 0x02 : 0x00)) | ((sector >> 24) & 0x0f));    if (b->flags & B_DIRTY) {
        outb(base_port, write_cmd);
        outsl(0x1f0, b->data, BSIZE / 4);
    } else {
        outb(base_port, read_cmd);
    }
}

// Interrupt handler.
void
ideintr(void) {
    struct buf *b;

    // First queued buffer is the active request.
    acquire(&idelock);

    if((b = idequeue) != 0) {
        idequeue = b->qnext;
    } else if((b = idequeue2) != 0) {
        idequeue2 = b->qnext;
    }

    if ((b = idequeue) == 0) {
        release(&idelock);
        return;
    }
    idequeue = b->qnext;

    // Read data if needed.
    if (!(b->flags & B_DIRTY) && idewait(b->dev,1) >= 0)
        insl(0x1f0, b->data, BSIZE / 4);

    // Wake process waiting for this buf.
    b->flags |= B_VALID;
    b->flags &= ~B_DIRTY;
    wakeup(b);

    // Start disk on next buf in queue.
    if (idequeue != 0)
        idestart(b->dev, idequeue);

    release(&idelock);
}

//PAGEBREAK!
// Sync buf with disk. pri and flag is not status swap, increment priority to avoid an infinite loop
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b, uint dev) {

    struct buf **pp;
    struct spinlock *lock;
    struct buf *queue;

    if(b->dev == 2 ) {
        lock = &idelock2;
        queue = idequeue2;
    } else{
        lock = &idelock;
        queue = idequeue;
    }


    if (!holdingsleep(&b->lock))
        panic("iderw: buf not locked");
    if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID)
        panic("iderw: nothing to do");
    if (b->dev != 0 && (!havedisk1 && !havedisk2))
        panic("iderw: ide disk 1 and 2 not present");

    acquire(lock);  //DOC:acquire-lock

    // Append b to idequeue.
    b->qnext = 0;
    for (pp = &queue; *pp; pp = &(*pp)->qnext)  //DOC:insert-queue
        ;
    *pp = b;

    // Start disk if necessary.
    if (queue == b)
        idestart(b->dev, b);

    // Wait for request to finish.
    while ((b->flags & (B_VALID | B_DIRTY)) != B_VALID) {
        sleep(b, lock);
    }


    release(lock);
}
