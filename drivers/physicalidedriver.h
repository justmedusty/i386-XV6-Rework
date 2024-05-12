//
// Created by dustyn on 5/12/24.
//

#ifndef XV6I386_PHYSICALIDEDRIVER_H
#define XV6I386_PHYSICALIDEDRIVER_H
#include "x86.h"

// Define IDE controller registers
#define IDE_REG_DATA        0x1F0
#define IDE_REG_ERROR       0x1F1
#define IDE_REG_SECTOR_COUNT 0x1F2
#define IDE_REG_LBA_LOW     0x1F3
#define IDE_REG_LBA_MID     0x1F4
#define IDE_REG_LBA_HIGH    0x1F5
#define IDE_REG_DEVICE      0x1F6
#define IDE_REG_COMMAND     0x1F7

// Define IDE commands
#define IDE_CMD_WRITE       0x30

// Define sector size
#define IDE_SECTOR_SIZE     512

void ide_select_device(unsigned char device);
void ide_write_register(unsigned short reg, unsigned char  value);
void ide_read_register(unsigned short reg, unsigned char  *value);
void ide_wait_busy();
void ide_wait_ready();


#endif //XV6I386_PHYSICALIDEDRIVER_H
