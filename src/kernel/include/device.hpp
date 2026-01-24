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

enum DEVICE_TYPE {
    ATA,
    AHCI
};

class StorageDevice {
protected:
    char model[41];
    char serial[21];
    char firmware[9];
    uint32_t total_sectors;
public:
    virtual void read(void* buffer, uint64_t lba, uint64_t sectors = 1U) = 0;
    virtual void write(void* buffer, uint64_t lba, uint64_t sectors = 1U) = 0;
};

class AtaDevice : public StorageDevice {
private:
    // Hardware identifier
    ata::Bus bus;     // Primary / Secondary
    ata::Drive drive; // Master / Slave 
public:
    AtaDevice(ata::Bus bus, ata::Drive drive) : bus(bus), drive(drive) {}

    void read(void* buffer, uint64_t lba, uint64_t sectors = 1U) override;
    void write(void* buffer, uint64_t lba, uint64_t sectors = 1U) override;
};

class AhciDevice : public StorageDevice {
private:
    AhciDriver* ahci;
    HBA_PORT* port;
public:
    AhciDevice(AhciDriver* driver, HBA_PORT* port) : ahci(driver), port(port) {}

    void read(void* buffer, uint64_t lba, uint64_t sectors = 1U) override;
    void write(void* buffer, uint64_t lba, uint64_t sectors = 1U) override;
};

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
