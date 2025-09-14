// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#ifndef MBR_HPP
#define MBR_HPP

#include <stdint.h>
#include <device.hpp>

struct mbr_partition_entry {
    uint8_t status;          // 0x80 bootable
    uint8_t chs_first[3];    // ignored
    uint8_t type;            // 0x83 = Linux
    uint8_t chs_last[3];     // ignored
    uint32_t lba_start;      // first sector of partition
    uint32_t sector_count;   // total sectors in partition
} __attribute__((packed));

struct mbr_t {
    uint8_t bootloader[446];
    mbr_partition_entry partitions[4];
    uint16_t signature;      // 0x55AA
} __attribute__((packed));

namespace mbr {
    bool read_mbr(ata::device_t* dev, mbr_t* mbr);
    uint32_t find_partition_lba(mbr_t* mbr);
} // namespace mbr

#endif // MBR_HPP