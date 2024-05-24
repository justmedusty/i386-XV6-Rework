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
#define IDE_CMD_RDMUL 0xc4
#define IDE_CMD_WRMUL 0xc5

#define BASEPORT1     0x1f0
#define BASEPORT2     0x170

// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.
// You must hold idelock while manipulating queue.

static struct spinlock idelock;
static struct buf *idequeue;
static struct buf *idequeue2;


static int havedisk1;
static int havedisk2;

static void idestart(uint dev, struct buf *);

// Wait for IDE disk to become ready.
static void idestart(uint dev, struct buf *);

// Wait for IDE disk to become ready.
static int idewait(int dev, int checkerr) {
    int r;
    int port = (dev == 2) ? BASEPORT2 + 7 : BASEPORT1 + 7;
    cprintf("Waiting for disk on port: %x\n", port);

    while (((r = inb(port)) & (IDE_BSY | IDE_DRDY)) != IDE_DRDY) {
        cprintf("Status: %x on port: %x\n", r, port);
        if (r & 0xff) {
            panic("disk error/no disk");
        }
        if (checkerr && (r & (IDE_DF | IDE_ERR)) != 0) {
            return -1;
        }
    }
    return 0;
}

void
ideinit(void) {

    int i;

    initlock(&idelock, "ide");
    ioapicenable(IRQ_IDE, ncpu - 1);
    ioapicenable(IRQ_IDE2, ncpu - 1);

    if (idewait(1, 0) == -1) {
        panic("idewait1");
    }

    // Check if disk 1 is present
    outb(BASEPORT1 + 6, 0xe0 | (1 << 4));
    for (i = 0; i < 1000; i++) {
        if (inb(BASEPORT1 + 7) != 0) {
            havedisk1 = 1;
            break;
        }
    }

    if (idewait(2, 0) == -1) {
        panic("idewait2");
    }

    // Check if disk 2 is present
    outb(BASEPORT2 + 6, 0xe0 | (0 << 4)); // Select disk 2 (slave on second IDE channel)
    for (i = 0; i < 1000; i++) {
        if (inb(BASEPORT2 + 7) != 0) { // Check status register for disk 2
            cprintf("%d\n",inb(BASEPORT2 + 7));
            havedisk2 = 1;
            break;
        }
    }

    if (!havedisk1) {
        panic("Missing disk 1");
    }

    if (!havedisk2) {
        panic("Missing disk 2");
    }


    // Switch back to disk 0.
    outb(BASEPORT1 + 6, 0xe0 | (0 << 4));

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
    read_cmd = (sector_per_block == 1) ? IDE_CMD_READ : IDE_CMD_RDMUL;
    write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;
    // Select the appropriate command based on the disk
    if (b->dev == 1) {
        base_port = BASEPORT1;
    } else {
        base_port = BASEPORT2;
    }

    if (sector_per_block > 7) panic("idestart");

    idewait(b->dev, 0);
    outb(base_port + 206, 0);  // generate interrupt
    outb(base_port + 2, sector_per_block);  // number of sectors
    outb(base_port + 3, sector & 0xff);
    outb(base_port + 4, (sector >> 8) & 0xff);
    outb(base_port + 5, (sector >> 16) & 0xff);
    outb(base_port + 6, 0xe0 | ((b->dev & 1) << 4) | ((sector >> 24) & 0x0f));
    if (b->flags & B_DIRTY) {

        outb(base_port + 7, write_cmd);
        outsl(base_port, b->data, BSIZE / 4);
    } else {
        outb(base_port + 7, read_cmd);
    }
}

// Interrupt handler.
// Interrupt handler.
void
ideintr(void) {
    struct buf *b;

    // First queued buffer is the active request.
    acquire(&idelock);


    if (((b = idequeue) == 0) && (b = idequeue2) == 0) {
        release(&idelock);
        return;
    }
    idequeue = b->qnext;

    // Read data if needed.
    if (!(b->flags & B_DIRTY) && idewait(b->dev, 1) >= 0) {
        int baseport = (b->dev == 1) ? BASEPORT1 : BASEPORT2;
        insl(baseport, b->data, BSIZE / 4);

    }

    // Wake process waiting for this buf.
    b->flags |= B_VALID;
    b->flags &= ~B_DIRTY;
    wakeup(b);


    if ((b = idequeue) != 0) {
        idestart(b->dev, idequeue);
    } else if ((b = idequeue2) != 0) {
        idestart(b->dev, idequeue2);
    }
    idequeue = b->qnext;

    release(&idelock);

}

//PAGEBREAK!
// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b, uint dev) {
    struct buf **pp;

    struct spinlock *lock;
    struct buf *queue;

    if (b->dev == 2) {
        lock = &idelock;
        queue = idequeue2;
    } else {
        lock = &idelock;
        queue = idequeue;
    }

    if (!holdingsleep(&b->lock))
        panic("iderw: buf not locked");
    if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID)
        panic("iderw: nothing to do");
    if (b->dev != 0 && !havedisk1)
        panic("iderw: ide disk 1 not present");
    if (b->dev != 0 && !havedisk2)
        panic("iderw: ide disk 2 not present");

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