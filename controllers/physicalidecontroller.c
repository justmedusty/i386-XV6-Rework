//
// Created by dustyn on 5/12/24.
//

/*
 * Primary ide controller,  I would like to eventually run this on hardware so I will setu p the controller and drivers now.
 *
 * It will follow the standard ide standards obviously,
 * There will only be a primary ide controller there will be
 * no secondary ide controllers as I do not think this is necessary
 * for a system such as xv6
 */
#include "x86.h"


#include "physicalidecontroller.h"
#include "../drivers/physicalidedriver.h"

void ide_init(struct ide_controller *ide) {
    // Initialize the IDE controller ports
    ide->data_port = IDE_DATA_PORT_PRIMARY;
    ide->error_port = IDE_ERROR_PORT_PRIMARY;
    ide->sector_count_port = IDE_SECTOR_COUNT_PORT_PRIMARY;
    ide->lba_low_port = IDE_LBA_LOW_PORT_PRIMARY;
    ide->lba_mid_port = IDE_LBA_MID_PORT_PRIMARY;
    ide->lba_high_port = IDE_LBA_HIGH_PORT_PRIMARY;
    ide->device_port = IDE_DEVICE_PORT_PRIMARY;
    ide->command_port = IDE_COMMAND_PORT_PRIMARY;
    ide->control_port = IDE_CONTROL_PORT_PRIMARY;
}

void ide_read_sector(struct ide_controller *ide, unsigned lba, unsigned char *buffer) {
    // Select the drive (master/slave)
    ide_write_register(ide->device_port, 0xE0 | ((lba >> 24) & 0xF));

    // Send sector count
    ide_write_register(ide->sector_count_port, 1);

    // Send LBA address
    ide_write_register(ide->lba_low_port, (unsigned char) lba);
    ide_write_register(ide->lba_mid_port, (unsigned char) (lba >> 8));
    ide_write_register(ide->lba_high_port, (unsigned char) (lba char >> 16));

    // Send command
    ide_write_register(ide->command_port, 0x20);

    // Wait for the disk to be ready
    while (ide_read_register(ide->command_port) & (1 << 7)) {}

    // Read data from disk
    insl(ide->data_port, buffer, 512 / 4);
}

void ide_write_sector(struct ide_controller *ide, unsigned int lba, unsigned char *buffer) {
    // Select the drive (master/slave)
    ide_write_register(ide->device_port, 0xE0 | ((lba >> 24) & 0xF));

    // Send sector count
    ide_write_register(ide->sector_count_port, 1);

    // Send LBA address
    ide_write_register(ide->lba_low_port, (unsigned char) lba);
    ide_write_register(ide->lba_mid_port, (unsigned char) (lba >> 8));
    ide_write_register(ide->lba_high_port, (unsigned char) (lba >> 16));

    // Send command
    ide_write_register(ide->command_port, 0x30);

    // Wait for the disk to be ready
    while (ide_read_register(ide->command_port) & (1 << 7)) {}

    // Write data to disk
    outsl(ide->data_port, buffer, 512 / 4);
}
