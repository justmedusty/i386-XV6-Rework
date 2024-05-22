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


    int result = mount(dev,path);

    if(result != 0){

        switch (result) {
            case -EMOUNTPNTLOCKED:
                printf(1,"Mount point locked\n");
                exit();

            case -EMOUNTNTDIR:
                printf(1,"Mount point not a directory\n");
                exit();


            case -EMNTPNTNOTFOUND:
                printf(1,"No inode at pathname was found\n");
                exit();


        }


    }

    printf(1,"Mount point now on %s\n",argv[2]);
    exit();


}