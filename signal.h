//
// Created by dustyn on 5/8/24.
//

#ifndef XV6_ORIGINAL_SIGNAL_H
#define XV6_ORIGINAL_SIGNAL_H

//terminal hangup
#define SIGHUP  1
//interrupt
#define SIGINT  2
//illegal memory access (cannot be ignored)
#define SIGSEG  4
//kill (cannot be ignored)
#define SIGKILL 8
//io on pipe with no reader / writer (cannot be ignored)
#define SIGPIPE 16
//bad system call
#define SIGSYS 32
//time quantum exceeded
#define SIGCPU 64



#endif //XV6_ORIGINAL_SIGNAL_H
