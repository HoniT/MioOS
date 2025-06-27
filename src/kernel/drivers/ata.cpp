// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// ata.cpp
// ATA/IDE driver for communicating with IDE disk
// ========================================

#include <drivers/ata.hpp>
#include <device.hpp>
#include <pic.hpp>
#include <interrupts/idt.hpp>
#include <drivers/vga_print.hpp>
#include <io.hpp>
#include <lib/string_util.hpp>

using namespace io;

// Initializes ATA driver for 28-bit PIO mode
void ata::init(void) {
    // Probing ATA
    ata::probe();
}

// Checks all 4 devices
bool ata::probe(void) {
    ata::Bus buses[] = {ata::Bus::Primary, ata::Bus::Secondary};
    ata::Drive drives[] = {ata::Drive::Master, ata::Drive::Slave};
    bool found_device = false;

    // Itterating thro buses and drives
    for(ata::Bus bus : buses)
        for(ata::Drive drive : drives)
            // ata::identify already ads device so no need to add here
            if(ata::identify(bus, drive)) found_device = true;
        
    return found_device;
}

// Identifies if an ATA device exists
bool ata::identify(Bus bus, Drive drive) {
    bool secondary = (bus == Bus::Secondary);
    bool slave = (drive == Drive::Slave);
    
    // Getting corresponding ports for the bus and drive fields given
    uint16_t drive_head_port = secondary ? SECONDARY_DRIVE_HEAD : PRIMARY_DRIVE_HEAD;
    uint16_t sector_count_port = secondary ? SECONDARY_SECTOR_COUNT : PRIMARY_SECTOR_COUNT;
    uint16_t sector_num_port = secondary ? SECONDARY_SECTOR_NUM : PRIMARY_SECTOR_NUM;
    uint16_t lba_low_port = secondary ? SECONDARY_LBA_LOW : PRIMARY_LBA_LOW;
    uint16_t lba_high_port = secondary ? SECONDARY_LBA_HIGH : PRIMARY_LBA_HIGH;
    uint16_t command_port = secondary ? SECONDARY_COMMAND : PRIMARY_COMMAND;
    uint16_t status_port = secondary ? SECONDARY_STATUS : PRIMARY_STATUS;
    uint16_t data_port = secondary ? SECONDARY_DATA : PRIMARY_DATA;
    
    /* Selects target drive by sending 0xA0 to master drive, or 0xB0 to the
     * slave drives "drive select" IO port */
    outPortB(drive_head_port, (slave ? 0xB0 : 0xA0));
    outPortB(sector_count_port, 0x0);
    outPortB(sector_num_port, 0x0);
    outPortB(lba_low_port, 0x0);
    outPortB(lba_high_port, 0x0);
    // Sending IDENTIFY command to command IO port
    outPortB(command_port, IDENTIFY_COMMAND);

    vga::printf("Checking %s bus, %s drive: ", secondary ? "Secondary" : "Primary", slave ? "Slave" : "Master");
    // If we couldn't find an ATA device
    if (inPortB(status_port) == 0) {
        vga::warning("ATA device couldn't be found!\n");
        return false;
    }

    // Polling status port until bit 7 (BSY (0x80)) clears
    while (inPortB(status_port) & 0x80);

    // If LBAmid and LBAhi ports are non-zero the device is not ATA
    if (inPortB(lba_low_port) == 0x14 && inPortB(lba_high_port) == 0xEB) {
        vga::warning("Device was ATAPI or SATA!\n");
        return false;
    }

    /* Continue polling until bit 3 (DRQ (0x8)) of status port is set
     * or until bit 0 (ERR (0x1)) is set */
    while (true) {
        uint8_t status = inPortB(status_port);
        if (status & 0x08) break; // DRQ set, OK
        if (status & 0x01) {
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

    // Saving to device
    ata::save_ata_device(buffer, bus, drive);

    vga::printf("ATA device successfully found!\n");
    return true;
}

namespace pio_28 {
    
    // Reads one sector from lba on the given bus and drive and saves value to buffer
    void read_one_sector(ata::Bus bus, ata::Drive drive, uint32_t lba, uint16_t* buffer) {
        bool secondary = (bus == ata::Bus::Secondary);
        bool slave = (drive == ata::Drive::Slave);
        
        // Getting corresponding ports for the bus and drive fields given
        uint16_t data_port = secondary ? SECONDARY_DATA : PRIMARY_DATA;
        uint16_t sector_count_port = secondary ? SECONDARY_SECTOR_COUNT : PRIMARY_SECTOR_COUNT;
        uint16_t sector_num_port = secondary ? SECONDARY_SECTOR_NUM : PRIMARY_SECTOR_NUM;
        uint16_t lba_low_port = secondary ? SECONDARY_LBA_LOW : PRIMARY_LBA_LOW;
        uint16_t lba_high_port = secondary ? SECONDARY_LBA_HIGH : PRIMARY_LBA_HIGH;
        uint16_t drive_head_port = secondary ? SECONDARY_DRIVE_HEAD : PRIMARY_DRIVE_HEAD;
        uint16_t command_port = secondary ? SECONDARY_COMMAND : PRIMARY_COMMAND;
        
        // Sending 0xE0 to master drive , ORed with the highest 4 bits of LBA
        uint8_t drive_head = (slave ? 0xF0 : 0xE0) | ((lba >> 24) & 0x0F);
        outPortB(drive_head_port, drive_head);
        
        // Sending sector count (in this case only reading 1 sector at a time)
        outPortB(sector_count_port, 0x1);
        outPortB(sector_num_port, (uint8_t)(lba));
        outPortB(lba_low_port, (uint8_t)((lba) >> 8));
        outPortB(lba_high_port, (uint8_t)((lba) >> 16));
        
        // Sending READ SECTORS command (0x20)
        outPortB(command_port, READ_SECTOR_COMMAND);
        
        // Waiting for IRQ or poll (in this case waiting for DRQ (0x8) to set)
        while (!(inPortB(command_port) & 0x08));
        
        // Writing to buffer
        for (int j = 0; j < 256; j++) {
            buffer[j] = inPortW(data_port);
        }
    }
    
    // Reads a given amount of sectors starting at a given LBA from a given device
    bool read_sector(ata::device_t* dev, uint32_t lba, uint16_t* buffer, uint32_t sectors) {
        if(strlen(dev->serial) == 0) {
            vga::warning("Invalid device passed to READ!\n");
            return false;
        }

        for(int i = 0; i < sectors; i++) {
            read_one_sector(dev->bus, dev->drive, lba + i, buffer);
            buffer += 512;
        }
        return true;
    }

    // Writes value given from buffer into lba on given bus and drive
    void write_one_sector(ata::Bus bus, ata::Drive drive, uint32_t lba, uint16_t* buffer) {
        bool secondary = (bus == ata::Bus::Secondary);
        bool slave = (drive == ata::Drive::Slave);
        
        // Getting corresponding ports for the bus and drive fields given
        uint16_t data_port = secondary ? SECONDARY_DATA : PRIMARY_DATA;
        uint16_t sector_count_port = secondary ? SECONDARY_SECTOR_COUNT : PRIMARY_SECTOR_COUNT;
        uint16_t sector_num_port = secondary ? SECONDARY_SECTOR_NUM : PRIMARY_SECTOR_NUM;
        uint16_t lba_low_port = secondary ? SECONDARY_LBA_LOW : PRIMARY_LBA_LOW;
        uint16_t lba_high_port = secondary ? SECONDARY_LBA_HIGH : PRIMARY_LBA_HIGH;
        uint16_t drive_head_port = secondary ? SECONDARY_DRIVE_HEAD : PRIMARY_DRIVE_HEAD;
        uint16_t command_port = secondary ? SECONDARY_COMMAND : PRIMARY_COMMAND;
        uint16_t status_port = secondary ? SECONDARY_STATUS : PRIMARY_STATUS;
        
        // Wait for the drive to be ready
        while (inPortB(status_port) & 0x80); // Wait for BSY (0x80) to clear
        
        // Send drive/head: 0xE0 = master, LBA mode
        uint8_t drive_head = (slave ? 0xF0 : 0xE0) | ((lba >> 24) & 0x0F);
        outPortB(drive_head_port, drive_head);
        
        // Selecting one sector
        outPortB(sector_count_port, 1);
        outPortB(sector_num_port, (uint8_t)(lba));
        outPortB(lba_low_port, (uint8_t)((lba) >> 8));
        outPortB(lba_high_port, (uint8_t)((lba) >> 16));
        
        // Send WRITE SECTORS command (0x30)
        outPortB(command_port, WRITE_SECTOR_COMMAND);
        
        // Wait for DRQ (data request)
        while (!(inPortB(status_port) & 0x08)); // Wait for DRQ set
        
        // Write 256 words (512 bytes) to the data port
        for (int j = 0; j < 256; j++) {
            outPortW(data_port, buffer[j]);
        }
        
        // Flush cache with 0xE7 command (optional but recommended)
        outPortB(command_port, 0xE7);
        
        // Wait for BSY to clear again after flush
        while (inPortB(status_port) & 0x80);
    }

    // Writes a value to a given amount of sectors starting at a given LBA from a given device
    bool write_sector(ata::device_t* dev, uint32_t lba, uint16_t* buffer, uint32_t sectors) {
        if(strlen(dev->serial) == 0) {
            vga::warning("Invalid device passed to WRITE!\n");
            return false;
        }

        for(int i = 0; i < sectors; i++) {
            write_one_sector(dev->bus, dev->drive, lba + i, buffer);
            buffer += 512;
        }
        return true;
    }

} // namespace pio_28