//
// Created by dustyn on 6/11/24.
//
#include "../../user/types.h"
#include "../defs/defs.h"
#include "hash.h"
//get a 16 bit hash
unsigned short hash_16(int value){
    value *= HASH_CONSTANT_1;
    value ^= (value >> 16);
    value &= HASH_CONSTANT_2;
    value ^= (value >> 13);
    value &= 0xFFFF;
    return value;
}

//get an 8 bit hash
unsigned char hash_8(int value){
    value *= HASH_CONSTANT_1;
    value ^= (value >> 16);
    value *= HASH_CONSTANT_2;
    value ^= (value >>11);
    value ^= HASH_CONSTANT_1 + HASH_CONSTANT_2;
    value &= 0xFF;
    return value;
}


void hash_test_8(){
    int hash_table[256];
    int collisions;
    for(int i=0;i < 256;i++){
        hash_table[i]= 0;
    }
    for(int i=1;i < 512;i++){
        unsigned char j = hash_8(~i  * hash_8(~i));
        if(hash_table[j] != 0){
            collisions++;
        }
        hash_table[j]= 1;
    }
        cprintf("%d collisions! %d\n",collisions);

}