//
// Created by dustyn on 5/9/24.
//


/*
 * User executable for the sig sys call to be run from the terminal, send a signal of a certain id. Right now there are no users,
 * no permissions etc. there is only kernel or user so this can be used on any other process. At some point I will add users and groups and permission bits etc.
 */
#include "syscall.h"
#include "types.h"
#include "user.h"
#include "signal.h"


void sig_handler(int sig_id){

    if(sig_id == SIGINT){
        printf(1,"RECEIVED INTERRUPT\n");
    } else{
        printf(1,"RECEIVED OTHER SIGNAL\n");
    }
}

int main(int argc,char **argv) {
    if (argc < 3 || argc > 3) {
        printf(2, "Usage : sig sig_id pid\n");
        exit();
    }

    sighandler((void*) &sig_handler);


    int result = sig(atoi(argv[1]), atoi(argv[2]));


    if(result == ENOPROC) {
        printf(2, "Process not found! pid \n");
        return ENOPROC;
    }

    if(result == ESIG){
        printf(2,"Bad signal id!\n");
        return ESIG;
    }


    printf(1,"Signal %d sent to pid %d\n", atoi(argv[1]), atoi(argv[2]));
    exit();
}