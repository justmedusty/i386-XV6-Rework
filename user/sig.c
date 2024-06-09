//
// Created by dustyn on 5/9/24.
//


/*
 * User executable for the sig sys call to be run from the terminal, send a signal of a certain id. Right now there are no users,
 * no permissions etc. there is only kernel or user so this can be used on any other process. At some point I will add users and groups and permission bits etc.
 */
#include "../kernel/syscall/syscall.h"
#include "types.h"
#include "user.h"
#include "../kernel/sched/signals.h"


void *sig_handler() {
    printf(1, "RECEIVED INTERRUPT\n");
    return;
}

int main(int argc, char **argv) {
    if (argc == 2 && ((strcmp(argv[1],"--list")) == 0) ){
        printf(1,"SIGUP   : %d\n",SIGHUP);
        printf(1,"SIGINT  : %d\n",SIGINT);
        printf(1,"SIGSEG  : %d\n",SIGSEG);
        printf(1,"SIGKILL : %d\n",SIGKILL);
        printf(1,"SIGPIPE : %d\n",SIGPIPE);
        printf(1,"SIGSYS  : %d\n",SIGSYS);
        printf(1,"SIGCPU  : %d\n",SIGCPU);
        exit();

    } else if (argc < 3 || argc > 3) {{
        printf(2, "Usage : sig sig_id pid or sig --list\n");
        exit();
        }

    }


    sighandler(sig_handler);


    int result = sig(atoi(argv[1]), atoi(argv[2]));


    if (result == ENOPROC) {
        printf(2, "Process not found! pid \n");
        return ENOPROC;
    }

    if (result == ESIG) {
        printf(2, "Bad signal id!\n");
        return ESIG;
    }


    printf(1, "Signal %d sent to pid %d\n", atoi(argv[1]), atoi(argv[2]));
    exit();
}