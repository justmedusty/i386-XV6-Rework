//
// Created by dustyn on 5/22/24.
//

#include "user.h"
#include "syscall.h"


int main(int argc, char *argv[]) {

    if (argc < 2 || argc > 2) {
        printf(1, "Usage : umountfs directory\n");
        exit();
    }

    char *path = argv[1];
    int result = umount(path);

    if (result != 0) {
        switch (result) {

            case -EMOUNTPOINTBUSY:
                printf(1, "Mount point busy\n");
                exit();

            case -ENOMOUNT:
                printf(1, "Mount point does not exist\n");
                exit();

            default:
                printf(1, "Error occurred\n");
                exit();
        }

    }
    printf(1, "Mount point removed on %s\n", argv[1]);
    exit();
}