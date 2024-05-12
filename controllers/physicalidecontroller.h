//
// Created by dustyn on 5/12/24.
//

#ifndef XV6I386_PHYSICALIDECONTROLLER_H
#define XV6I386_PHYSICALIDECONTROLLER_H
/*
 * For the ide controller below, the data port, error port, sector count, low med and high ports,
 * dev port prim, command and control ports
 */

struct ide_controller {
    unsigned short data_port;
    unsigned short error_port;
    unsigned short sector_count_port;
    unsigned short lba_low_port;
    unsigned short lba_mid_port;
    unsigned short lba_high_port;
    unsigned short device_port;
    unsigned short command_port;
    unsigned short control_port;
};

#define IDE_DATA_PORT_PRIMARY 0x1F0
#define IDE_ERROR_PORT_PRIMARY 0x1F1
#define IDE_SECTOR_COUNT_PORT_PRIMARY 0x1F2
#define IDE_LBA_LOW_PORT_PRIMARY 0x1F3
#define IDE_LBA_MID_PORT_PRIMARY 0x1F4
#define IDE_LBA_HIGH_PORT_PRIMARY 0x1F5
#define IDE_DEVICE_PORT_PRIMARY 0x1F6
#define IDE_COMMAND_PORT_PRIMARY 0x1F7
#define IDE_CONTROL_PORT_PRIMARY 0x3F6


#endif //XV6I386_PHYSICALIDECONTROLLER_H
