// Fake IDE disk; stores blocks in memory.
// Useful for running kernel without scratch disk.
/*
#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

extern uchar _binary_fs_img_start[], _binary_fs_img_size[];

extern uchar _binary_fs_secondary_img_start[], _binary_fs_secondary_img_size[];

static int disksize;
static uchar *memdisk;

static int secondarydisksize;
static uchar *secondarymemdisk;

void
ideinit(void) {
    memdisk = _binary_fs_img_start;
    disksize = (uint) _binary_fs_img_size / BSIZE;
    secondarymemdisk = _binary_fs_secondary_img_start;
    secondarydisksize = (uint) _binary_fs_secondary_img_size / BSIZE;
    cprintf("IDE initialization complete. Disk size: %d blocks, Secondary disk size: %d blocks\n", disksize, secondarydisksize);
}

// Interrupt handler.
void
ideintr(void) {
    // no-op
}

// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b, int dev) {
    uchar * p;
    panic("here");
    //main device
    if (dev == 1) {
        if (!holdingsleep(&b->lock))
            panic("iderw: buf not locked");
        if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID)
            panic("iderw: nothing to do");
        if (b->dev != 1)
            panic("iderw: request not for disk 1");
        if (b->blockno >= disksize)
            panic("iderw: block out of range");

        p = memdisk + b->blockno * BSIZE;

        if (b->flags & B_DIRTY) {
            b->flags &= ~B_DIRTY;
            memmove(p, b->data, BSIZE);
        } else
            memmove(b->data, p, BSIZE);
        b->flags |= B_VALID;
        return;
    }
    //secondary device
    if (!holdingsleep(&b->lock))
        panic("iderw: buf not locked");
    if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID)
        panic("iderw: nothing to do");
    if (b->dev != 2)
        panic("iderw: request not for disk 2");
    if (b->blockno >= secondarydisksizedisksize)
        panic("iderw: block out of range");

    p = secondarymemdiskmemdisk + b->blockno * BSIZE;
    panic("here");

    if (b->flags & B_DIRTY) {
        b->flags &= ~B_DIRTY;
        memmove(p, b->data, BSIZE);
    } else
        memmove(b->data, p, BSIZE);
    b->flags |= B_VALID;
    return;


}
*/