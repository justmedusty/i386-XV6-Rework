//
// Created by dustyn on 5/22/24.
//

#include "user.h"
#include "syscall.h"


int main(int argc, char *argv[]){

    if(argc < 3 || argc > 3 ){
        printf(1,"Usage : mount device directory\n");
        exit();
    }

    int dev = atoi(argv[1]);
    char *path = argv[2];

    moun


}