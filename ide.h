//
// Created by dustyn on 5/28/24.
//

#ifndef XV6_I386_IDE_H
#define XV6_I386_IDE_H
/*
 * Bitmask for querying existence of devices outside of the ide driver
 */
#define DEV1 0x1
#define DEV2 0x2
#define DEV3 0x4
#define DEV4 0x8
#define DEV5 0x10
#define DEV6 0x20
#define DEV7 0x40
#define DEV8 0x80

char disk_query();
#endif //XV6_I386_IDE_H
