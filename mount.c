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
struct inode* mount(uint dev, char path*){

    begin_op();

    struct inode *mountpoint = namei(dev,path);

    if(mountpoint == 0 || mountpoint->type != T_DIR){
        return 0;
    }


    if(!acquirenonblockinglock(&mountlock)){
        //Dont spin or sleep just return if the lock is taken
        return 0;
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
        return -1;
    }

    struct inode *mountp = mounttable->mount_point;
    mountp->is_mount_point = 0;


}