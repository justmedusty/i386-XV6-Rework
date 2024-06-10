// Simple PIO-based (non-DMA) IDE driver code.

#include "../../user/types.h"
#include "../defs/defs.h"
#include "../defs/param.h"
#include "../arch/x86_32/mem/memlayout.h"
#include "../arch/x86_32/mem/mmu.h"
#include "../lock/spinlock.h"
#include "../sched/proc.h"
#include "../arch/x86_32/x86.h"
#include "../arch/x86_32/traps.h"
#include "../lock/sleeplock.h"
#include "../fs/fs.h"
#include "../fs/buf.h"
#include "ide.h"

#define SECTOR_SIZE   512
#define IDE_BSY       0x80
#define IDE_DRDY      0x40
#define IDE_DF        0x20
#define IDE_ERR       0x01

#define IDE_CMD_READ  0x20
#define IDE_CMD_WRITE 0x30
#define IDE_CMD_RDMUL 0xc4
#define IDE_CMD_WRMUL 0xc5


#define DISK1 0x1  // ata0 master (xv6.img)
#define DISK2 0x2  // ata0 slave (secondaryfs.img)
#define DISK3 0x4  // ata1 master (secondaryfs.img)
#define DISK4 0x8  // ata1 slave (unimplemented)
#define DISK5 0x10 // ata2 master (unimplemented)
#define DISK6 0x20 // ata2 slave (unimplemented)
#define DISK7 0x40 // ata3 master (unimplemented)
#define DISK8 0x80 // ata3 slave (unimplemented)

#define BASEPORT1     0x1f0
#define BASEPORT2     0x170
#define BASEPORT3     0x1e8
#define BASEPORT4     0x168

#define CONTROLBASE1  0x3f6
#define CONTROLBASE2  0x376
#define CONTROLBASE3  0x3e6
#define CONTROLBASE4  0x366


// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.
// You must hold idelock while manipulating queue.

static struct spinlock idelock;
static struct spinlock idelock2;
static struct buf *idequeue;
static struct buf *idequeue2;

unsigned char disk_presence;


// Wait for IDE disk to become ready.
static void idestart(uint dev, struct buf *);

/*
 * Simple function that returns a char bitmask indicating which disks are present, will be useful for santizing random device numbers passed as parameters.
 */
char disk_query(){
    return disk_presence;
}

// Wait for IDE disk to become ready.
static int idewait(int dev, int checkerr) {
    int r;
    int port = (dev == 2) ? BASEPORT2 + 7 : BASEPORT1 + 7;

    /*
     * I have to send the ident cmd to the secondary ata controller for some reason, I did not have to do this
     * for the first disk. I am not sure why. But anyway, this is why this is here.
     */
    if (!(disk_presence & DISK3) || !((disk_presence & DISK4)) || !((disk_presence & DISK5)) || !((disk_presence & DISK6))|| !((disk_presence & DISK7)) || !((disk_presence & DISK8))) {
        outb(BASEPORT2 + 6, 0xe0 | (0 << 4));
    }

    while (((r = inb(port)) & (IDE_BSY | IDE_DRDY)) != IDE_DRDY) {

        if (r & 0xff) {
            cprintf("Error on port : %x on dev: %x\n", inb(port), dev);
        }
        if (r == 0) {
            cprintf("Disk not found on dev: %x on port: %x\n", dev, port);
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
    disk_presence = 0;
    ioapicenable(IRQ_IDE, ncpu - 1);
    ioapicenable(IRQ_IDE2, ncpu - 1);
    idewait(1, 0);
    idewait(2, 0);

    //Check if disk 0 (ata0 master) is present
    outb(BASEPORT1 + 6, 0xe0 | (0 << 4));
    for (i = 0; i < 1000; i++) {
        if (inb(BASEPORT1 + 7) != 0) {
            disk_presence |= DISK1;
            cprintf("Found disk 0 master : %x\n", inb(BASEPORT1 + 7));
            break;
        }
    }

    // Check if disk 1 (ata0 slave) is present
    outb(BASEPORT1 + 6, 0xe0 | (1 << 4));
    for (i = 0; i < 1000; i++) {
        if (inb(BASEPORT1 + 7) != 0) {
            disk_presence |= DISK2;
            cprintf("Found disk 0 slave : %x\n", inb(BASEPORT1 + 7));
            break;
        }
    }


    // Check if disk 2 (ata1 master) is present
    outb(BASEPORT2 + 6, 0xe0 | (0 << 4));
    for (i = 0; i < 1000; i++) {
        if (inb(BASEPORT2 + 7) != 0) {
            disk_presence |= DISK3;
            cprintf("Found disk 1 master : %x\n", inb(BASEPORT2 + 7));
            break;
        }
    }

    // Check if disk 3 (ata1 slave) is present
    outb(BASEPORT2 + 6, 0xe0 | (1 << 4));
    for (i = 0; i < 1000; i++) {
        if (inb(BASEPORT2 + 7) != 0) {
            disk_presence |= DISK4;
            cprintf("Found disk 1 slave : %x\n", inb(BASEPORT2 + 7));
            break;
        }
    }

    // Check if disk 4 (ata2 master) is present
    outb(BASEPORT3 + 6, 0xe0 | (0 << 4));
    for (i = 0; i < 1000; i++) {
        if (inb(BASEPORT3 + 7) != 0) {
            disk_presence |= DISK5;
            cprintf("Found disk 2 master : %x\n", inb(BASEPORT3 + 7));
            break;
        }
    }

    // Check if disk 5 (ata2 slave) is present
    outb(BASEPORT3 + 6, 0xe0 | (1 << 4));
    for (i = 0; i < 1000; i++) {
        if (inb(BASEPORT3 + 7) != 0) {
            disk_presence |= DISK6;
            cprintf("Found disk 2 master : %x\n", inb(BASEPORT3 + 7));
            break;
        }
    }

    // Check if disk 6 (ata3 master) is present
    outb(BASEPORT4 + 6, 0xe0 | (0 << 4));
    for (i = 0; i < 1000; i++) {
        if (inb(BASEPORT4 + 7) != 0) {
            disk_presence |= DISK7;
            cprintf("Found disk 2 master : %x\n", inb(BASEPORT4 + 7));
            break;
        }
    }

    // Check if disk 6 (ata3 master) is present
    outb(BASEPORT4 + 6, 0xe0 | (1 << 4));
    for (i = 0; i < 1000; i++) {
        if (inb(BASEPORT4 + 7) != 0) {
            disk_presence |= DISK8;
            cprintf("Found disk 2 master : %x\n", inb(BASEPORT4 + 7));
            break;
        }
    }



    // Switch back to disk 0.
    outb(BASEPORT1 + 7, 0xe0 | (0 << 4));

    if (!(disk_presence & DISK1)) {
        panic("Missing disk 1");
    }
    if (!(disk_presence & DISK2))  {
        panic("Missing disk 2");
    }
    if (!(disk_presence & DISK3))  {
        panic("Missing disk 3");
    }


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
    int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ : IDE_CMD_RDMUL;
    int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;

    if (sector_per_block > 7) panic("idestart");


    if (dev == 1) {
        idewait(1, 0);
        outb(CONTROLBASE1, 0);  // generate interrupt
        outb(BASEPORT1 + 2, sector_per_block);  // number of sectors
        outb(BASEPORT1 + 3, sector & 0xff);
        outb(BASEPORT1 + 4, (sector >> 8) & 0xff);
        outb(BASEPORT1 + 5, (sector >> 16) & 0xff);
        outb(BASEPORT1 + 6, 0xe0 | ((b->dev & 1) << 4) | ((sector >> 24) & 0x0f));
        if (b->flags & B_DIRTY) {
            outb(BASEPORT1 + 7, write_cmd);
            outsl(BASEPORT1, b->data, BSIZE / 4);
        } else {
            outb(BASEPORT1 + 7, read_cmd);
        }

    } else if (dev == 2) {
        idewait(2, 0);
        outb(CONTROLBASE2, 0);  // generate interrupt
        outb(BASEPORT2 + 2, sector_per_block);  // number of sectors
        outb(BASEPORT2 + 3, sector & 0xff);
        outb(BASEPORT2 + 4, (sector >> 8) & 0xff);
        outb(BASEPORT2 + 5, (sector >> 16) & 0xff);
        outb(BASEPORT2 + 6, 0xe0 | (0 << 4) | ((sector >> 24) & 0x0f));
        if (b->flags & B_DIRTY) {
            outb(BASEPORT2 + 7, write_cmd);
            outsl(BASEPORT2, b->data, BSIZE / 4);
        } else {
            outb(BASEPORT2 + 7, read_cmd);

        }
    }

}

// Interrupt handler.
// Interrupt handler.
void
ideintr(void) {
    struct buf *b;

    // First queued buffer is the active request.
    acquire(&idelock);

    if ((b = idequeue) == 0) {
        release(&idelock);
        return;
    }
    idequeue = b->qnext;

    // Read data if needed.
    if (!(b->flags & B_DIRTY) && idewait(b->dev, 1) >= 0) {
        int baseport = BASEPORT1;
        insl(baseport, b->data, BSIZE / 4);

    }

    // Wake process waiting for this buf.
    b->flags |= B_VALID;
    b->flags &= ~B_DIRTY;
    wakeup(b);


    if ((b = idequeue) != 0) {
        idestart(b->dev, idequeue);

        idequeue = b->qnext;

        release(&idelock);

    }
    release(&idelock);
}

void
ideintr2(void) {
    struct buf *b;

    // First queued buffer is the active request.
    acquire(&idelock2);


    if ((b = idequeue2) == 0) {
        release(&idelock);
        return;
    }
    idequeue2 = b->qnext;

    // Read data if needed.
    if (!(b->flags & B_DIRTY) && idewait(2, 1) >= 0) {
        int baseport = BASEPORT2;
        insl(baseport, b->data, BSIZE / 4);

    }

    // Wake process waiting for this buf.
    b->flags |= B_VALID;
    b->flags &= ~B_DIRTY;
    wakeup(b);


    if ((b = idequeue2) != 0) {
        idestart(b->dev, idequeue2);
        idequeue2 = b->qnext;
    }




    release(&idelock2);

}

//PAGEBREAK!
// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b, uint dev) {
    struct buf **pp;

    if (!holdingsleep(&b->lock))
        panic("iderw: buf not locked");
    if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID)
        panic("iderw: nothing to do");

    if (dev == 1) {
        acquire(&idelock);  //DOC:acquire-lock

        // Append b to idequeue.
        b->qnext = 0;
        for (pp = &idequeue; *pp; pp = &(*pp)->qnext)  //DOC:insert-queue
            ;
        *pp = b;

        // Start disk if necessary.
        if (idequeue == b)
            idestart(1, b);

        // Wait for request to finish.
        while ((b->flags & (B_VALID | B_DIRTY)) != B_VALID) {
            sleep(b, &idelock);
        }


        release(&idelock);
        return;
    } else if (dev == 2) {
        acquire(&idelock2);  //DOC:acquire-lock

        // Append b to idequeue.
        b->qnext = 0;
        for (pp = &idequeue2; *pp; pp = &(*pp)->qnext)  //DOC:insert-queue
            ;
        *pp = b;

        // Start disk if necessary.
        if (idequeue2 == b)
            idestart(2, b);
        // Wait for request to finish.
        while ((b->flags & (B_VALID | B_DIRTY)) != B_VALID) {
            sleep(b, &idelock2);
        }
        release(&idelock2);
        return;
    }

}