// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <stdint.h>
#include <drivers/ata.hpp>
#include <drivers/ahci.hpp>

extern ata::device_t** ata_devices;
extern uint8_t last_ata_device_index;
void device_init(void);

namespace ata {
    /* Do to my foolish actions ATA devices get to keep the name device_t instead of e.g. ata_device_t.
    * It would be to much work to rename the struct since im not using an IDE :( I'm sorry. */
   
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

namespace ahci {
    // AHCI device information
    struct device_t {
        char model[41];
        char serial[21];
        char firmware[9];
        uint64_t total_sectors;
        
        AhciDriver* ahci;
        HBA_PORT* port;
    };
    
    void save_ahci_device(char* model, char* serial, char* firmware, uint64_t sectors, AhciDriver* ahci, HBA_PORT* port);
} // namespace ahci
extern data::list<ahci::device_t*> ahci_devices;

#endif // DEVICE_HPP
