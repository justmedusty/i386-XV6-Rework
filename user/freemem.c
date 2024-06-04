//
// Created by dustyn on 5/7/24.
//
#include "../kernel/syscall/syscall.h"
#include "types.h"
#include "user.h"


int main(){
    int free_pages = freemem();
    printf(1,"Pages allocated : %d which amounts to %d bytes\n",free_pages,free_pages * 4096);
    exit();
};