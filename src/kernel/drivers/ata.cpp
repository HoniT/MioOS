// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// ata.cpp
// ATA/IDE driver for communicating with IDE disk
// ========================================

#include <drivers/ata.hpp>
#include <drivers/pit.hpp>
#include <device.hpp>
#include <x86/interrupts/pic.hpp>
#include <x86/interrupts/idt.hpp>
#include <graphics/vga_print.hpp>
#include <x86/io.hpp>
#include <lib/string_util.hpp>
#include <lib/data/string.hpp>

using namespace io;

// IRQ handlers
volatile bool primary_irq_received = false;
volatile bool secondary_irq_received = false;

void primary_ata_handler(InterruptRegisters* regs) {
    primary_irq_received = true;
    // Sending EOI
    pic::send_eoi(PRIMARY_IDE_IRQ);
}

void secondary_ata_handler(InterruptRegisters* regs) {
    secondary_irq_received = true;
    // Sending EOI
    pic::send_eoi(SECONDARY_IDE_IRQ);
}

void ata_irq_wait(const bool secondary) {
    volatile bool* irq_ptr = secondary ? &secondary_irq_received : &primary_irq_received;

    // Wait for IRQ with timeout
    int timeout = 2000;
    while (!*irq_ptr && --timeout) pit::delay(1);
    if (timeout <= 0) {
        kprintf(LOG_ERROR, "ATA IRQ timout\n");
        return;
    }

    *irq_ptr = false;
}

// Initializes ATA driver for 28-bit PIO mode
void ata::init(void) {

    // Switching to IRQ mode and subscribing handlers
    idt::irq_install_handler(PRIMARY_IDE_IRQ, &primary_ata_handler);
    outPortW(PRIMARY_DEVICE_CONTROL, 0x00);
    idt::irq_install_handler(SECONDARY_IDE_IRQ, &secondary_ata_handler);
    outPortW(SECONDARY_DEVICE_CONTROL, 0x00);
    
    // Unmasking IRQs
    pic::unmask_irq(PRIMARY_IDE_IRQ);
    pic::unmask_irq(SECONDARY_IDE_IRQ);

    // Probing ATA
    ata::probe();

    if(!idt::check_irq(PRIMARY_IDE_IRQ, &primary_ata_handler) || !idt::check_irq(SECONDARY_IDE_IRQ, &secondary_ata_handler)) {
        kprintf(LOG_ERROR, "Failed to initialize ATA driver! (IRQ 14 and/or 15 not set)\n");
    }
    kprintf(LOG_INFO, "Implemented ATA driver to IRQ mode\n");
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
    ata::delay_400ns(secondary);
    outPortB(sector_count_port, 0x0);
    outPortB(sector_num_port, 0x0);
    outPortB(lba_low_port, 0x0);
    outPortB(lba_high_port, 0x0);
    // Sending IDENTIFY command to command IO port
    outPortB(command_port, IDENTIFY_COMMAND);
    ata::delay_400ns(secondary);

    // If we couldn't find an ATA device
    if (inPortB(status_port) == 0) {
        // kprintf("ATA device couldn't be found!\n");
        return false;
    }
    
    // Waiting for IRQ
    ata_irq_wait(secondary);
    
    // If LBAmid and LBAhi ports are non-zero the device is not ATA
    if (inPortB(lba_low_port) == 0x14 && inPortB(lba_high_port) == 0xEB) {
        // kprintf("Device was ATAPI or SATA!\n");
        return false;
    }
    
    /* Continue polling until bit 3 (DRQ (0x8)) of status port is set
    * or until bit 0 (ERR (0x1)) is set */
   while (true) {
       uint8_t status = inPortB(status_port);
       if (status & 0x08) break; // DRQ set, OK
       if (status & 0x01) {
           kprintf(LOG_ERROR, "Error while identifying ATA device!\n");
           return false;
        }
    }
    
    // If error is cleared data is ready to read from data port
    // Reading 256 words (256 x 2 = 512 bytes)
    uint16_t buffer[256];
    for (uint16_t i = 0; i < 256; i++) {
        buffer[i] = inPortW(data_port);
    }
    
    // Saving to device
    ata::save_ata_device(buffer, bus, drive);
    
    kprintf(LOG_INFO, RGB_COLOR_LIGHT_GRAY, "%s bus, %s drive: ", secondary ? "Secondary" : "Primary", slave ? "Slave" : "Master");
    kprintf("ATA device successfully found!\n");
    return true;
}

void ata::delay_400ns(const bool secondary) {
    uint16_t ctrl_port = secondary ? SECONDARY_STATUS : PRIMARY_STATUS;
    for(int i = 0; i < 4; i++) inPortB(ctrl_port);
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
        uint16_t status_port = secondary ? SECONDARY_STATUS : PRIMARY_STATUS;
        

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

        ata_irq_wait(secondary);

        // Waiting for BSY to clear
        while(inPortB(status_port) & 0x80);
        // Waiting for DRQ to set
        while(!(inPortB(status_port) & 0x8));
        
        // Writing to buffer
        for (int j = 0; j < 256; j++) {
            buffer[j] = inPortW(data_port);
        }
    }
    
    // Reads a given amount of sectors starting at a given LBA from a given device
    bool read_sector(ata::device_t* dev, uint32_t lba, uint16_t* buffer, uint32_t sectors) {
        if(strlen(dev->serial) == 0) {
            kprintf(LOG_WARNING, "Invalid device passed to READ!\n");
            return false;
        }

        for(int i = 0; i < sectors; i++) {
            read_one_sector(dev->bus, dev->drive, lba + i, buffer);
            buffer += 256;
        }
        return true;
    }

    // Writes value given from buffer into lba on given bus and drive
    void write_one_sector(ata::Bus bus, ata::Drive drive, uint32_t lba, uint16_t* buffer) {
        bool secondary = (bus == ata::Bus::Secondary);
        bool slave = (drive == ata::Drive::Slave);

        // Getting corresponding ports for the bus and drive fields given
        uint16_t data_port        = secondary ? SECONDARY_DATA        : PRIMARY_DATA;
        uint16_t sector_count_port= secondary ? SECONDARY_SECTOR_COUNT: PRIMARY_SECTOR_COUNT;
        uint16_t sector_num_port  = secondary ? SECONDARY_SECTOR_NUM  : PRIMARY_SECTOR_NUM; // aka LBA[7:0]
        uint16_t lba_mid_port     = secondary ? SECONDARY_LBA_LOW     : PRIMARY_LBA_LOW;   // aka LBA[15:8]
        uint16_t lba_high_port    = secondary ? SECONDARY_LBA_HIGH    : PRIMARY_LBA_HIGH;  // aka LBA[23:16]
        uint16_t drive_head_port  = secondary ? SECONDARY_DRIVE_HEAD  : PRIMARY_DRIVE_HEAD;
        uint16_t command_port     = secondary ? SECONDARY_COMMAND     : PRIMARY_COMMAND;
        uint16_t status_port      = secondary ? SECONDARY_STATUS      : PRIMARY_STATUS;

        // Send drive/head: 0xE0 = master, LBA mode; 0xF0 = slave, LBA mode
        // LBA[27:24] goes into the low 4 bits of this register
        uint8_t drive_head = (slave ? 0xF0 : 0xE0) | ((lba >> 24) & 0x0F);
        outPortB(drive_head_port, drive_head);

        // Required 400ns delay after drive select (spec says 4 status reads)
        inPortB(status_port); inPortB(status_port);
        inPortB(status_port); inPortB(status_port);

        // Selecting one sector
        outPortB(sector_count_port, 1);
        outPortB(sector_num_port, (uint8_t)(lba & 0xFF));        // LBA[7:0]
        outPortB(lba_mid_port,    (uint8_t)((lba >> 8) & 0xFF)); // LBA[15:8]
        outPortB(lba_high_port,   (uint8_t)((lba >> 16) & 0xFF));// LBA[23:16]

        // Send WRITE SECTORS command (0x30)
        outPortB(command_port, WRITE_SECTOR_COMMAND);

        // Waiting for BSY to clear and DRQ to set (with error checks)
        uint8_t status;
        do {
            status = inPortB(status_port);
            if (status & 0x01) return; // ERR bit set -> error
            if (status & 0x20) return; // DF bit set -> device fault
        } while ((status & 0x80) || !(status & 0x08));

        // Write 256 words (512 bytes) to the data port
        for (int j = 0; j < 256; j++) {
            outPortW(data_port, buffer[j]);
        }

        // Wait for IRQ
        ata_irq_wait(secondary);
    }

    void flush_cache(ata::Bus bus, ata::Drive drive) {
        bool secondary = (bus == ata::Bus::Secondary);
        uint16_t command_port = secondary ? SECONDARY_COMMAND : PRIMARY_COMMAND;
        uint16_t status_port  = secondary ? SECONDARY_STATUS  : PRIMARY_STATUS;

        // Send Flush Cache Command
        outPortB(command_port, 0xE7);

        while(inPortB(status_port) & 0x80);
    }

    // Writes a value to a given amount of sectors starting at a given LBA from a given device
    bool write_sector(ata::device_t* dev, uint32_t lba, uint16_t* buffer, uint32_t sectors) {
        if(strlen(dev->serial) == 0) {
            kprintf(LOG_WARNING, "Invalid device passed to WRITE!\n");
            return false;
        }

        for(int i = 0; i < sectors; i++) {
            write_one_sector(dev->bus, dev->drive, lba + i, buffer);
            buffer += 256;
        }

        if(sectors >= SECTORS_WRITTEN_FOR_CACHE_FLUSH) 
            pio_28::flush_cache(dev->bus, dev->drive);
        return true;
    }

} // namespace pio_28
