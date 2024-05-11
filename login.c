//
// Created by dustyn on 5/11/24.
// login shell.

#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "file.h"

int
getcmd(char *buf, int nbuf)
{
    printf(2, "$ ");
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if(buf[0] == 0) // EOF
        return -1;
    return 0;
}


int main(){
    static char buf[100];
    int fd;

    // Ensure that three file descriptors are open.
    while((fd = open("console", O_RDWR)) >= 0){
        if(fd >= 3){
            close(fd);
            break;
        }
    }
    printf(1,"Enter username\n");

};