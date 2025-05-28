// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// ata.cpp
// ATA/IDE driver for communicating with IDE disk
// ========================================

#include <drivers/ata.hpp>
#include <lib/io.hpp>
#include <drivers/vga_print.hpp>

using namespace io;

// Identifies if an ATA device exists
bool ata::identify(void) {
    /* Selects target drive by sending 0xA0 to master drive, or 0xB0 to the
     * slave drives "drive select" IO port */
    outPortB(PRIMARY_DRIVE_HEAD, 0xA0);
    outPortB(PRIMARY_SECTOR_COUNT, 0x0);
    outPortB(PRIMARY_SECTOR_NUM, 0x0);
    outPortB(PRIMARY_LBA_LOW, 0x0);
    outPortB(PRIMARY_LBA_HIGH, 0x0);
    // Sending IDENTIFY command to command IO port
    outPortB(PRIMARY_COMMAND, IDENTIFY_COMMAND);

    // If we couldnt find an ATA device
    if(inPortB(PRIMARY_STATUS) == 0) {
        vga::error("ATA device couldn't be found!\n");
        return false;
    }

    // Polling status port untill bit 7 (BSY (0x80)) clears
    while(inPortB(PRIMARY_STATUS) & 0x80);
    
    // If LBAmid and LBAhi ports are non-zero the device is not ATA
    if(inPortB(PRIMARY_LBA_LOW) == 0x14 && inPortB(PRIMARY_LBA_HIGH) == 0xEB) {
        vga::error("Device was ATAPI or SATA!\n");
        return false;
    }

    /* Continuing polling untill bit 3 (DRQ (0x8)) of status port is set
     * or untill bit 0 (ERR (0x1)) is set */
    while (true) {
        uint8_t status = inPortB(PRIMARY_STATUS);
        if (status & 0x08) break;  // DRQ set, OK
        if (status & 0x01) {       // ERR set
            vga::error("Error while identifying ATA device!\n");
            return false;
        }
    }


    // If error is cleared data is ready to read from data port
    // Reading 256 words (256 x 2 = 512 bytes)
    uint16_t buffer[256];
    for (uint16_t i = 0; i < 256; i++) {
        buffer[i] = inPortW(PRIMARY_DATA);
    }
    
    vga::printf("ATA device succesfully found!\n");
    return true;
}

// Reads sector from lba on the primary bus and saves value to buffer
void pio_28::read_sector(uint32_t lba, uint16_t* buffer, uint16_t sector_count) {
    for (uint16_t i = 0; i < sector_count; i++) {
        // Sending 0xE0 to master drive , ORed with the highest 4 bits of LBA
        outPortB(PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));

        // Sending sectorcount (in this case only reading 1 sector at a time)
        outPortB(PRIMARY_SECTOR_COUNT, 0x1);
        outPortB(PRIMARY_SECTOR_NUM, (uint8_t)(lba + i));
        outPortB(PRIMARY_LBA_LOW, (uint8_t)((lba + i) >> 8));
        outPortB(PRIMARY_LBA_HIGH, (uint8_t)((lba + i) >> 16));

        // Sending READ SECTORS command (0x20) to port 0x1F7
        outPortB(PRIMARY_COMMAND, READ_SECTOR_COMMAND);

        // Waiting for IRQ or poll (in this case waiting for DRQ (0x8) to set)
        while (!(inPortB(PRIMARY_COMMAND) & 0x08));

        // Writing to buffer
        for (int j = 0; j < 256; j++) {
            buffer[i * 256 + j] = inPortW(PRIMARY_DATA);
        }
    }
    return;
}

// Writes value given from buffer into lba on primary bus
void pio_28::write_sector(uint32_t lba, uint16_t* buffer, uint16_t sector_count) {
    for (uint16_t i = 0; i < sector_count; i++) {
        // Wait for the drive to be ready
        while (inPortB(PRIMARY_STATUS) & 0x80); // Wait for BSY (0x80) to clear

        // Send drive/head: 0xE0 = master, LBA mode
        outPortB(PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));

        // Selecting one sector
        outPortB(PRIMARY_SECTOR_COUNT, 1);
        outPortB(PRIMARY_SECTOR_NUM, (uint8_t)(lba + i));
        outPortB(PRIMARY_LBA_LOW, (uint8_t)((lba + i) >> 8));
        outPortB(PRIMARY_LBA_HIGH, (uint8_t)((lba + i) >> 16));

        // Send WRITE SECTORS command (0x30)
        outPortB(PRIMARY_COMMAND, WRITE_SECTOR_COMMAND);

        // Wait for DRQ (data request)
        while (!(inPortB(PRIMARY_STATUS) & 0x08)); // Wait for DRQ set

        // Write 256 words (512 bytes) to the data port
        for (int j = 0; j < 256; j++) {
            outPortW(PRIMARY_DATA, buffer[i * 256 + j]);
        }
    }

    // Flush cache with 0xE7 command (optional but recommended)
    outPortB(PRIMARY_COMMAND, 0xE7);

    // Wait for BSY to clear again after flush
    while (inPortB(PRIMARY_STATUS) & 0x80);
}
