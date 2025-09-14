// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// mbr.cpp
// Defines MBR structure and MBR partitioning functions
// ========================================

#include <mbr.hpp>
#include <mm/heap.hpp>
#include <lib/mem_util.hpp>
#include <drivers/vga_print.hpp>

// Validates MBR
bool mbr::read_mbr(ata::device_t* dev, mbr_t* mbr) {
    if(!dev || !mbr) return false;

    uint16_t buffer[256]; // 512 bytes / 2
    pio_28::read_sector(dev, 0, buffer, 1); // Read sector 0
    memcpy(mbr, buffer, sizeof(mbr_t));
    
    if(mbr->signature != 0xAA55) return false;
    return true;
}

// Finds partition LBA start
uint32_t mbr::find_partition_lba(mbr_t* mbr) {
    if(!mbr) return 0;

    for(int i = 0; i < 4; i++) {
        if(mbr->partitions[i].type == 0x83) { // Ext2
            return mbr->partitions[i].lba_start;
        }
    }
    return 0; // None found
}