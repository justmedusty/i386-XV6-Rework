//
// Created by dustyn on 5/11/24.
// login shell.

#include "types.h"
#include "user.h"
#include "fcntl.h"

char *argv[] = { "sh", 0 };

#define MAX_USER_LEN 40
#define MAX_PASSWD_LEN 40
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

    static char username[50];
    static char password[50];

    // Ensure that three file descriptors are open.
    while((fd = open("console", O_RDWR)) >= 0){
        if(fd >= 3){
            close(fd);
            break;
        }
    }
    printf(1,"Enter username\n");
    getcmd(buf, MAX_USER_LEN);
    memmove(username,buf, strlen(buf));

    printf(1,"Enter password\n");
    change_mode(1);
    getcmd(buf, MAX_PASSWD_LEN);
    memmove(password,buf, strlen(buf));

    change_mode(0);


    int pid = fork();
    if(pid < 0){
        printf(1, "init: fork failed\n");
        exit();
    }
    if(pid == 0){
        exec("sh", argv);
        printf(1, "init: exec sh failed\n");
        exit();
    }






};