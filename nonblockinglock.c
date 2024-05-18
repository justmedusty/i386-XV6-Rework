//
// Created by dustyn on 5/18/24.
//

/*
 * I will use this lock for mounts and maybe other things as well, for now it will just indicate that the point is mounted so the same mount point cannot be mounted again.
 * It will not affect a processes ability to unmount , only to mount the same point twice.
 */

#include "nonblockinglock.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"

void
initnonblockinglock(struct nonblockinglock *lk, char *name)
{
    initlock(&lk->lk, "nonblocking lock");
    lk->name = name;
    lk->locked = 0;
}

int
acquirenonblockinglock(struct nonblockinglock *lk)
{
    acquire(&lk->lk);
    if(lk->locked) {
        //immediately return, no sleeping or spinning
        return 0;
    }
    lk->locked = 1;
    release(&lk->lk);
    return 1;
}

void
releasenonblocking(struct nonblockinglock *lk)
{
    acquire(&lk->lk);
    lk->locked = 0;
    release(&lk->lk);
}



