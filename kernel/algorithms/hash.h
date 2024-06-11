//
// Created by dustyn on 6/11/24.
//

#ifndef I386_XV6_REWORK_HASH_H
#define I386_XV6_REWORK_HASH_H
//hash constants (large primes)
#define HASH_CONSTANT_1 2796203
#define HASH_CONSTANT_2 2017963
#define HASH_CONSTANT_3 10619863
#define HASH_CONSTANT_4 7772777
#define HASH_CONSTANT_5 77477
#define HASH_CONSTANT_6 1398269

unsigned short hash_16(int value);
unsigned char hash_8(int value);
void hash_test_8();

#endif //I386_XV6_REWORK_HASH_H
