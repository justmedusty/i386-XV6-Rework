//
// Created by dustyn on 5/18/24.
//

#include "mount.h"

struct superblock sb;


struct devsw devsw[NDEV];


struct inode* mount(int dev, struct inode *mountpoint){
}