//
// Created by dustyn on 5/9/24.
//


/*
 * User executable for the sig sys call to be run from the terminal, send a signal of a certain id
 */
#include "syscall.h"
#include "types.h"
#include "user.h"


int main(int argc,char *argv[]) {
    if (argc < 3 || argc > 3) {
        printf(2, "Usage : sig sig_id pid\n");
        exit();
    }

    int result = sig(*argv[1],*argv[2]);

    if(result == ENOPROC){
        printf(2,"Process not found!\n");
        return ENOPROC;
    }

    if(result == ESIG){
        printf(2,"Bad signal id!\n");
        return ESIG;
    }


    printf(1,"Signal sent to pid %d\n", *argv[2]);
    return 0;
}