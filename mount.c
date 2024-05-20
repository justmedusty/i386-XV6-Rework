//
// Created by dustyn on 5/18/24.
//
\

/*
 * I am adding mounting of secondary file systems. I am creating a non blocking lock
 * for this. I do not want any spinning or sleeping as with the sleeplock provided with XV6.
 * Reason being : If a mount point is busy, it is likely to be busy for a while, possibly until the system shuts down.
 * It does not make sense to have a sleeping or spinning process for an indeterminate amount of time. Just return and let
 * the process know it is busy. It can try again later, after all.
 */
#include "mount.h"
struct superblock sb;


struct nonblockinglock mountlock;


void init_mount_lock(){
    initnonblockinglock(mountlock,"mountlock");
}
/*
 * The mount function for our mounting functionality. It will take a path mountpoint
 */
struct inode* mount(uint dev, char path*){

    begin_op();
    struct inode *mountpoint = namei(dev,path);
    if(mountpoint->type != T_DIR){
        return -EMOUNTNTDIR;
    }
    if(mountpoint == 0){
        return -EMNTPNTNOTFOUND;
    }
    if(!acquirenonblockinglock(&mountlock)){
        //Dont spin or sleep just return if the lock is taken
        return -EMOUNTPNTLOCKED;
    }
    ilock(mountpoint);
    mountpoint->is_mount_point = 1;
    mounttable->lock = &mountlock;
    mounttable->mount_point = &mountpoint;
    readsb(dev,&sb);
    struct inode *mountroot = namei(dev,'/');
    if(mountroot == 0){
        return 0;
    }
    mounttable->mount_root = &mountroot;
    end_op();

}

int unmount(char *mountpoint){

    begin_op();

    if(mounttable.mount_point == 0){
        end_op();
        return -ENOMOUNT;
    }
    if(!acquirenonblockinglock(&mountlock)){
        return -ENOMOUNT;
    }
    if(iunlockputmount(mounttable.mount_root) != 0){
        return -EMOUNTPOINTBUSY
    }

    mounttable.mount_point->is_mount_point = 0;
    releasenonblocking(&mountlock);
    return 0;

}