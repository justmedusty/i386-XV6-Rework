//
// Created by dustyn on 5/18/24.
//

#include "mount.h"

struct superblock sb;


struct devsw devsw[NDEV];

struct {
    struct spinlock lock;
    struct inode mount_point;
    struct inode mount_root;
} mounttable;
