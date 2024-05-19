//
// Created by dustyn on 5/18/24.
//

#include "mount.h"

struct superblock sb;


struct nonblockinglock mountlock;


void init_mount_lock(){
    initnonblockinglock(mountlock,"mountlock");
}

struct inode* mount(int dev, struct inode *mountpoint,int dev *mountroot){

    if(!acquirenonblockinglock(&mountlock)){
        //Dont spin or sleep just return if the lock is takenb
        return 0;
    }

    mountpoint->is_mount_point = 1;
    mounttable->lock = &mountlock;
    mounttable->mount_point = &mountpoint;

    readsb(dev,&sb2);
    struct inode *mountroot =
    mounttable->mount_root = &mountroot;


}