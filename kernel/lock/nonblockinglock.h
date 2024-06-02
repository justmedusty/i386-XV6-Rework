//
// Created by dustyn on 5/18/24.
//

#ifndef XV6I386_NONBLOCKINGLOCK_H
#define XV6I386_NONBLOCKINGLOCK_H
//This will be used initially for mount point locking, it will just return if the lock is held since spinning or sleeping
// waiting for a mount point to be free is silly. It will likely be held for a while.
struct nonblockinglock {
    int locked;       // Is the lock held?
    struct spinlock lk; // spinlock protecting this nonblocking lock
    char *name;        // Name of lock.
};


#endif //XV6I386_NONBLOCKINGLOCK_H
