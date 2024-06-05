//
// Created by dustyn on 5/18/24.
//


/*
 * I am adding mounting of secondary file systems. I am creating a non blocking lock
 * for this. I do not want any spinning or sleeping as with the sleeplock provided with XV6.
 * Reason being: If a mount point is busy, it is likely to be busy for a while, possibly until the system shuts down.
 * It does not make sense to have a sleeping or spinning process for an indeterminate amount of time. Just return and let
 * the process know it is busy. It can try again later, after all.
 */

#include "../../user/types.h"
#include "fs.h"
#include "../defs/defs.h"
#include "../defs/param.h"
#include "../lock/spinlock.h"
#include "../lock/sleeplock.h"
#include "file.h"
#include "../lock/nonblockinglock.h"
#include "../../user/stat.h"
#include "mount.h"
#include "../drivers/ide.h"
#include "../arch/x86_32/mem/mmu.h"
#include "../sched/proc.h"

struct superblock superblock;


struct nonblockinglock mountlock;

struct mounttable mounttable = {NULL, NULL, NULL};

void init_mount_lock() {
    initnonblockinglock(&mountlock, "mountlock");
}

/*
 * The mount function for our mounting functionality. It will take a path mountpoint
 */
int mount(uint dev, char *path) {
    begin_op();


    char diskmask = disk_query();

    switch (dev) {
        case 1:
            return -ECANNOTMOUNTONMAIN;
        case 2:
            if (diskmask & DEV2) {
                break;
            }
            goto bad;
        case 3:
            if (diskmask & DEV3) {
                break;
            }
            goto bad;
        case 4:
            if (diskmask & DEV4) {
                break;
            }
            goto bad;
        case 5:
            if (diskmask & DEV5) {
                break;
            }
            goto bad;
        case 6:
            if (diskmask & DEV6) {
                break;
            }
            goto bad;
        case 7:
            if (diskmask & DEV7) {
                break;
            }
            goto bad;

        default:
            if (dev > 7) {
                return -EDEVOOR;
            }
        bad:
            return -ENODEV;
    }


    if (!acquirenonblockinglock(&mountlock)) {
        //Dont spin or sleep just return if the lock is taken
        end_op();
        return -EMOUNTPNTLOCKED;
    }

    struct inode *mountpoint = namei(1, path);


    if (mountpoint == 0) {
        end_op();
        releasenonblocking(&mountlock);
        return -EMNTPNTNOTFOUND;
    }

    //must be a directory, cannot mount on a file or device
    //The problem to needing the hackjob solution above without writing new functions for inode traversal without a path is changing the cwd to
    //mount point and then just using . as a path so it will go from cwd. It maybe makes more sense to write new functions than to do this
    //but for now this is ok.
    if (mountpoint->type != T_DIR) {
        struct inode *old_cwd = myproc()->cwd;
        myproc()->cwd = idup(mountpoint);
        //now that we swapped our cwd out , . will select the relative entry that will allow this mount to work properly.
        struct inode *dir_check = namei(mountpoint->dev,".");
        if(dir_check && dir_check->type == T_DIR){
            mountpoint = idup(dir_check);
            iput(dir_check);
            myproc()->cwd = old_cwd;
            goto fixed;

        }
        myproc()->cwd = old_cwd;
        //dont try to put a null pointer
        if(dir_check){
            iput(dir_check);
        }
        end_op();
        releasenonblocking(&mountlock);
        return -EMOUNTNTDIR;
    }

    fixed:

    if (mountpoint->inum == ROOTINO) {
        iput(mountpoint);
        end_op();
        releasenonblocking(&mountlock);
        return -ECANNOTMOUNTONROOT;

    }


    struct inode *mountroot = namei(dev, "/");

    if (mountroot == 0 || mountroot < 0) {
        releasenonblocking(&mountlock);
        end_op();
        return -EMOUNTROOTNOTFOUND;
    }

    mountpoint->is_mount_point = 1;
    mounttable.lock = &mountlock.lk;
    mounttable.mount_point = mountpoint;
    mounttable.mount_root = mountroot;
    end_op();

    return 0;

}

/*
 * Unmount a filesystem, ensure is it actually mounted and that there is a lock present
 */
int unmount(char *mountpoint) {

    begin_op();

    struct inode *mp = namei(1,mountpoint);

    if (mp == 0) {
        end_op();
        return -ENOMOUNT;
    }
    if(mounttable.mount_root != mp){
        return -ENOMOUNT;
    }
    if (acquirenonblockinglock(&mountlock)) {
        releasenonblocking(&mountlock);
        return -ENOMOUNT;
    }
    //put the mount pointer so we can reduce the ref count to what SHOULD be 1 (just the mount table inode) which we will check for below in iputmount
    iput(mp);
    if (iputmount(mounttable.mount_root) != 0) {
        cprintf("ref count : %d\n",mp->ref);

        return -EMOUNTPOINTBUSY;
    }
    mounttable.mount_point->is_mount_point = 0;
    mounttable.mount_point = 0;
    mounttable.mount_root = 0;
    releasenonblocking(&mountlock);
    return 0;

}