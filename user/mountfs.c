//
// Created by dustyn on 5/22/24.
//

#include "user.h"
#include "../kernel/syscall/syscall.h"


int main(int argc, char *argv[]){
    if(argc < 3 || argc > 3 ){
        printf(1,"Usage : mount device directory\n");
        exit();
    }
    int dev = atoi(argv[1]);
    char *path = argv[2];

    if(*path != '/'){
        printf(1,"Must start from root '/'\n");
        exit();
    }
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

            case -EMOUNTROOTNOTFOUND:
                printf(1,"Mount root not found\n");
                exit();

            case -EDEVOOR:
                printf(1,"Dev number out of range\n");
                exit();

            case -ENODEV:
                printf(1,"Device not found\n");
                exit();

            case -ECANNOTMOUNTONMAIN:
                printf(1,"Cannot mount main device onto main device\n");
                exit();


        }


    }

    printf(1,"Mount point now on %s\n",argv[2]);
    exit();


}