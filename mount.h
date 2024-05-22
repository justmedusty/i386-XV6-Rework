//
// Created by dustyn on 5/18/24.
//

#ifndef XV6I386_MOUNT_H
#define XV6I386_MOUNT_H
#define ENOMOUNT                2
#define EMOUNTNTDIR             3
#define EMNTPNTNOTFOUND         4
#define EMOUNTPNTLOCKED         5
#define EMOUNTPOINTBUSY         6
#define ECANNOTMOUNTONROOT      7


/*
 * Will only allow 1 mount point for now
 */
struct mounttable{
    struct spinlock *lock;
    struct inode *mount_point;
    struct inode *mount_root;
};

extern struct mounttable mounttable;
void init_mount_lock();
#endif //XV6I386_MOUNT_H
