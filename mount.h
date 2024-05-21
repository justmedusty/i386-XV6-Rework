//
// Created by dustyn on 5/18/24.
//

#ifndef XV6I386_MOUNT_H
#define XV6I386_MOUNT_H


/*
 * Will only allow 1 mount point for now
 */
struct {
    struct spinlock lock;
    struct inode *mount_point;
    struct inode *mount_root;
} mounttable;
#endif //XV6I386_MOUNT_H
