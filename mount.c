//
// Created by dustyn on 5/18/24.
//

#include "mount.h"
struct superblock sb;


struct nonblockinglock mountlock;


void init_mount_lock(){
    initnonblockinglock(mountlock,"mountlock");
}
/*
 * The mount function for our mounting functionality. It will take an inode moinpoint
 */
struct inode* mount(int dev, struct inode *mountpoint){

    if(!acquirenonblockinglock(&mountlock)){
        //Dont spin or sleep just return if the lock is taken
        return 0;
    }
    begin_op();
    ilock(mountpoint);

    mountpoint->is_mount_point = 1;
    mounttable->lock = &mountlock;
    mounttable->mount_point = &mountpoint;

    readsb(dev,&sb);
    struct inode *mountroot = namei(dev,'/');
    mounttable->mount_root = &mountroot;
    end_op();

}