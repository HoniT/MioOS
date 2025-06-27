// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <stdint.h>
#include <drivers/ata.hpp>

extern ata::device_t* ata_devices;
void device_init(void);

namespace ata {
    // ATA device information
    struct device_t {
        char model[41];
        char serial[21];
        char firmware[9];
        uint32_t total_sectors;
        bool lba_support;
        bool dma_support;

        // Hardware identifier
        ata::Bus bus;     // Primary / Secondary
        ata::Drive drive; // Master / Slave 
    };

    void save_ata_device(uint16_t* identify_data, const ata::Bus bus, const ata::Drive drive);
} // namespace ata

#endif // DEVICE_HPP
