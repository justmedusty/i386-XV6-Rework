//
// Created by dustyn on 5/11/24.
// login shell.

#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "syscall.h"

char *argv[] = {"sh", 0};

#define MAX_USER_LEN 40
#define MAX_PASSWD_LEN 40

/*
 * Will verify the creds out of the passwd file
 */

int verify_credentials(char username[MAX_USER_LEN], char password[MAX_PASSWD_LEN]) {
    int fd = open("/passwd", O_RDONLY);
    if (fd < 0) {
        printf(1, "Error: Unable to open /passwd file\n");
        return -1;
    }

    char entry[MAX_USER_LEN + MAX_PASSWD_LEN + 1]; // Add space for the separator and null terminator
    int len_username = strlen(username);
    int len_password = strlen(password);

    if (len_username >= MAX_USER_LEN || len_password >= MAX_PASSWD_LEN) {
        printf(1, "Error: Username or password too long\n");
        close(fd);
        return -1;
    }

    memmove(entry, username, len_username);
    memmove(entry + len_username, password, len_password);
    entry[len_username + 1 + len_password] = '\0';

    for(int i=0;i< strlen(entry);i++){
        if(entry[i] == '\n' || entry[i] == '\r' ){
            entry[i] = ' ';
        }
    }
    printf(1, "Entry: %s\n", entry);

    // Rest of your code for verifying credentials goes here...

    close(fd);
    return 0;
}
/*
 * Taken from the sh file , gets the cmd and packs it into the buffer via gets invocation
 */
int
getcmd(char *buf, int nbuf) {
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if (buf[0] == 0) // EOF
        return -1;
    return 0;
}

/*
 * Main login shell entry
 */

int main() {
    static char buf[100];
    int fd;

    static char username[50];
    static char password[50];

    int attempts = 0;

    // Ensure that three file descriptors are open.
    while ((fd = open("console", O_RDWR)) >= 0) {
        if (fd >= 3) {
            close(fd);
            break;
        }
    }
    start:
        if(attempts > 5){
            for(;;){
                //lock them out
            }
        }
        printf(1, "Enter username\n");
        getcmd(buf, MAX_USER_LEN);
        memmove(username, buf, strlen(buf));
        username[strlen(username)] = '\0';

        printf(1, "Enter password\n");
        changeconsmode(1);
        getcmd(buf, MAX_PASSWD_LEN);
        changeconsmode(0);
        memmove(password, buf, strlen(buf));


    
        int result = (verify_credentials(username,password);
        if(result == 0){
            goto finish;
        } else{
            attempts++
            goto start;
        }
    finish:
        printf(1,"Login success, starting shell proc\n");
        exec("sh", argv);
        printf(1, "init: exec sh failed\n");
        exit();

};