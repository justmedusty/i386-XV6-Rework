//
// Created by dustyn on 5/18/24.
//

#include "mount.h"

struct superblock sb;


struct devsw devsw[NDEV];
struct nonblockinglock mountlock;


void init_mount_lock(){
    initnonblockinglock(mountlock,"mountlock");
}

struct inode* mount(int dev, struct inode *mountpoint){

    if(!acquirenonblockinglock(&mountlock)){
        //Dont spin or sleep just return if the lock is takenb
        return 0;
    }

    mountpoint->is_mount_point = 1;

    mounttable->lock = &mountlock;
    mounttable->mount_point = &mountpoint;


}