// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef ATA_HPP
#define ATA_HPP

#include <stdint.h>
#include <lib/data/list.hpp>
#include <lib/data/string.hpp>

#define IDENTIFY_COMMAND 0xEC
#define READ_SECTOR_COMMAND 0x20
#define WRITE_SECTOR_COMMAND 0x30

// Registers for primary ATA bus
#define PRIMARY_DATA 0x1F0
#define PRIMARY_ERROR 0x1F1
#define PRIMARY_FEATURES 0x1F1
#define PRIMARY_SECTOR_COUNT 0x1F2
#define PRIMARY_SECTOR_NUM 0x1F3
#define PRIMARY_LBA_LOW 0x1F4
#define PRIMARY_LBA_HIGH 0x1F5
#define PRIMARY_DRIVE_HEAD 0x1F6
#define PRIMARY_STATUS 0x1F7
#define PRIMARY_COMMAND 0x1F7
// Device control register for primary ATA bus
#define PRIMARY_DEVICE_CONTROL 0x3F6

// Registers for secondary ATA bus
#define SECONDARY_DATA 0x170
#define SECONDARY_ERROR 0x171
#define SECONDARY_FEATURES 0x171
#define SECONDARY_SECTOR_COUNT 0x172
#define SECONDARY_SECTOR_NUM 0x173
#define SECONDARY_LBA_LOW 0x174
#define SECONDARY_LBA_HIGH 0x175
#define SECONDARY_DRIVE_HEAD 0x176
#define SECONDARY_STATUS 0x177
#define SECONDARY_COMMAND 0x177
// Device control register for secondary ATA bus
#define SECONDARY_DEVICE_CONTROL 0x376

// IRQ nums
#define PRIMARY_IDE_IRQ   14
#define SECONDARY_IDE_IRQ 15

namespace ata {
    struct device_t;

    enum class Bus {Primary, Secondary};
    enum class Drive {Master, Slave};

    // Initializes ATA driver for 28-bit PIO mode
    void init(void);
    // Identifies if an ATA device exists
    bool identify(Bus bus, Drive drive);
    // Checks all 4 devices
    bool probe(void);
    // 400 ns delay
    void delay_400ns(const bool secondary);
    
    // Terminal Functions
    void list_ata(data::list<data::string> params);
    void read_ata(data::list<data::string> params);
} // namespace ata

// 28 bit PIO mode functions
namespace pio_28 {
    // Reads a given amount of sectors starting at a given LBA from a given device
    bool read_sector(ata::device_t* dev, uint32_t lba, uint16_t* buffer, uint32_t sectors = 1);
    // Writes a value to a given amount of sectors starting at a given LBA from a given device
    bool write_sector(ata::device_t* dev, uint32_t lba, uint16_t* buffer, uint32_t sectors = 1);
} // namespace pio_28


#endif // ATA_HPP
