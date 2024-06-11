//
// Created by dustyn on 6/11/24.
//

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
    value &= HASH_CONSTANT_2;
    value ^= (value >> 13);
    value &= 0xFF;
    return value;
}