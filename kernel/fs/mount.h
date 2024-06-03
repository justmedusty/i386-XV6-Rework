//
// Created by dustyn on 5/18/24.
//

#ifndef XV6I386_MOUNT_H
#define XV6I386_MOUNT_H
#define ENOMOUNT                2 //unmount spot not a mountpoint
#define EMOUNTNTDIR             3 //mount point not directory
#define EMNTPNTNOTFOUND         4 //mount point not found
#define EMOUNTPNTLOCKED         5 //mount lock is taken
#define EMOUNTPOINTBUSY         6 //ref count too high
#define ECANNOTMOUNTONROOT      7 //cannot mount on dev 1 root
#define EMOUNTROOTNOTFOUND      8 //mount root not found , could be no filesystem on that device
#define ENODEV                  9 //dev not present
#define EDEVOOR                 10 //out of range
#define ECANNOTMOUNTONMAIN      11 // cannot mount main disk / drive (why would you mount the same device onto itself unless you like recursive mounting for some reason)


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
