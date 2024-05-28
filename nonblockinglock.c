//
// Created by dustyn on 5/18/24.
//

/*
 * I will use this lock for mounts and maybe other things as well, for now it will just indicate that the point is mounted so the same mount point cannot be mounted again.
 * It will not affect a processes ability to unmount , only to mount the same point twice.
 */
#include "types.h"
#include "spinlock.h"
#include "nonblockinglock.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "sleeplock.h"

void
initnonblockinglock(struct nonblockinglock *lock, char *name)
{
    initlock(&lock->lk, "nonblockinglock");
    lock->name = name;
    lock->locked = 0;
}

int
acquirenonblockinglock(struct nonblockinglock *lock)
{
    acquire(&lock->lk);
    if(lock->locked) {

        //immediately return, no sleeping or spinning
        release(&lock->lk);
        return 0;
    }
    lock->locked = 1;
    release(&lock->lk);
    return 1;
}

void
releasenonblocking(struct nonblockinglock *lock)
{
    acquire(&lock->lk);
    lock->locked = 0;
    release(&lock->lk);
}



