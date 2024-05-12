//
// Created by dustyn on 5/12/24.
//
#include "physicalidedriver.h"

/*
 * Physical ide driver for when I eventually get this version of xv6 onto hardware
 */

//Select the ide device
void ide_select_device(unsigned char device) {
    outb(IDE_REG_DEVICE, 0xE0 | (device << 4)); // Master or slave
}
//write passed value to the given register
void ide_write_register(unsigned short reg, unsigned char  value) {
    outb(reg, value);
}
//read specified register and assign its value to the passed value pointer
void ide_read_register(unsigned short reg, unsigned char *value) {
    *value = inb(reg);
}
//Tight loop waiting for the command register to free up, once the busy flag in the bitmask is clear loop will break
void ide_wait_busy() {
    while (ide_read_register(IDE_REG_COMMAND) & (1 << 7)) {} // Wait for busy flag to clear
}

//Wait for the ready flag to be set
void ide_wait_ready() {
    while (!(ide_read_register(IDE_REG_COMMAND) & (1 << 3))) {} // Wait for ready bit to be set
}