// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef ATA_HPP
#define ATA_HPP

#include <stdint.h>

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

namespace ata {
    // Identifies if an ATA device exists
    bool identify(void);
} // namespace ata

// 28 bit PIO mode functions
namespace pio_28 {
    // Reads sector from lba on the primary bus and saves value to buffer
    void read_sector(uint32_t lba, uint16_t* buffer, uint16_t sector_count = 1);
    // Writes value given from buffer into lba on primary bus
    void write_sector(uint32_t lba, uint16_t* buffer, uint16_t sector_count = 1);
} // namespace pio_28

#endif // ATA_HPP
