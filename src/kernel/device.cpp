// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// device.cpp
// In charge of storing storage (AHCI/ATA) devices
// ========================================

#include <device.hpp>
#include <mm/heap.hpp>
#include <graphics/vga_print.hpp>

ata::device_t** ata_devices;
uint8_t last_ata_device_index;

void device_init(void) {
    // Allocating memory for devices
    ata_devices = (ata::device_t**)kcalloc(4, sizeof(ata::device_t));
    last_ata_device_index = 0;
}

void ata::save_ata_device(uint16_t* data, const ata::Bus bus, const ata::Drive drive) {
    device_t* device = (device_t*)kmalloc(sizeof(device_t));

    // Convert strings from IDENTIFY data (they are in word-swapped format)

    // Serial
    for (int i = 0; i < 20; i++) {
        device->serial[i * 2]     = data[10 + i] >> 8;
        device->serial[i * 2 + 1] = data[10 + i] & 0xFF;
    }
    device->serial[20] = 0;

    // Firmware
    for (int i = 0; i < 4; i++) {
        device->firmware[i * 2]     = data[23 + i] >> 8;
        device->firmware[i * 2 + 1] = data[23 + i] & 0xFF;
    }
    device->firmware[8] = 0;

    // Device Model
    for (int i = 0; i < 20; i++) {
        device->model[i * 2]     = data[27 + i] >> 8;
        device->model[i * 2 + 1] = data[27 + i] & 0xFF;
    }
    device->model[40] = 0;

    // Capability flags
    device->lba_support = data[49] & (1 << 9);
    device->dma_support = data[49] & (1 << 8);

    // Total sectors (28-bit LBA)
    device->total_sectors = data[60] | (data[61] << 16);

    // Saving IO info
    device->bus = bus;
    device->drive = drive;

    // vga::printf("Saving ATA Device! model: %s, serial: %s, firmware: %s, total sectors: %u, lba_support: %h, dma_support: %h\n", 
    //     device->model, device->serial, device->firmware, device->total_sectors, (uint32_t)device->lba_support, (uint32_t)device->dma_support);
    // vga::printf("IO information: bus: %h, drive: %h\n", device->bus, device->drive);

    // Saving to array
    ata_devices[last_ata_device_index++] = device;
}

data::list<ahci::device_t*> ahci_devices = data::list<ahci::device_t*>();

void ahci::save_ahci_device(char* model, char* serial, char* firmware, uint64_t sectors, AhciDriver* ahci, HBA_PORT* port) {
    ahci::device_t* dev = (ahci::device_t*)kmalloc(sizeof(ahci::device_t)); 
    if (!dev) return;

    memcpy(dev->model, model, 40);
    dev->model[40] = 0; 
    memcpy(dev->serial, serial, 20);
    dev->serial[20] = 0;
    memcpy(dev->firmware, firmware, 8);
    dev->firmware[8] = 0;

    dev->total_sectors = sectors;
    
    // Save Hardware Info
    dev->ahci = ahci;
    dev->port = port;

    ahci_devices.add(dev);
}
