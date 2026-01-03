// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================
// pci.cpp
// PCI driver implementation (Configuration Space Access Mechanism #1)
// ========================================

#include <drivers/pci.hpp>
#include <x86/io.hpp>
#include <graphics/vga_print.hpp>
#include <lib/data/list.hpp>

// Helper to calculate the mechanism #1 address
static inline uint32_t get_config_address(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    /* * Construct the address for 0xCF8:
     * Bit 31: Enable (1)
     * Bit 23-16: Bus
     * Bit 15-11: Device
     * Bit 10-8: Function
     * Bit 7-2: Register Offset (must be 4-byte aligned, so we mask 0xFC)
     */
    return (uint32_t)((1 << 31) | 
                        ((uint32_t)bus << 16) | 
                        ((uint32_t)device << 11) | 
                        ((uint32_t)func << 8) | 
                        (offset & 0xFC)); // Force alignment to 4 bytes
}

/// @brief Reading from PCI config space
uint16_t pci::pci_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t register_offset) {
    uint32_t address = get_config_address(bus, device, func, register_offset);
    io::outPortL(PCI_CONFIG_ADDRESS, address);
    // Reading data
    return (uint16_t)((io::inPortL(PCI_CONFIG_DATA) >> ((register_offset & 2) * 8)) & 0xFFFF);
}

/// @brief Reading from PCI config space (32-bit)
uint32_t pci::pci_read32(uint8_t bus, uint8_t device, uint8_t func, uint8_t register_offset) {
    uint32_t address = get_config_address(bus, device, func, register_offset);
    io::outPortL(PCI_CONFIG_ADDRESS, address);
    // Reading data
    return io::inPortL(PCI_CONFIG_DATA);
}

/// @brief Writing 32-bit value to PCI config space
void pci::pci_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t register_offset, uint32_t value) {
    uint32_t address = get_config_address(bus, device, func, register_offset);
    io::outPortL(PCI_CONFIG_ADDRESS, address);
    io::outPortL(PCI_CONFIG_DATA, value);
}



uint16_t pci::pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t function) {
    // Vendor is at offset 0
    return pci::pci_read(bus, device, function, PCI_HEADER_0x0_VENDOR_ID);
}

uint8_t pci::pci_get_header_type(uint8_t bus, uint8_t device, uint8_t func) {
    // Header type is the lower 8 bits of the word at offset 0x0E
    return (uint8_t)(pci_read(bus, device, func, PCI_HEADER_0x0_HEADER_TYPE) & 0xFF);
}

uint8_t pci::pci_get_class_id(uint8_t bus, uint8_t device, uint8_t func) {
    return (uint8_t)(pci_read(bus, device, func, PCI_HEADER_0x0_CLASS) & 0xFF);
}

uint8_t pci::pci_get_subclass_id(uint8_t bus, uint8_t device, uint8_t func) {
    return (uint8_t)(pci_read(bus, device, func, PCI_HEADER_0x0_SUBCLASS) & 0xFF);
}


static data::list<pci_device_t> pci_devices = data::list<pci_device_t>();

/// @brief Checks for a specific function
void pci::pci_manage_function(uint8_t bus, uint8_t device, uint8_t function) {
    uint8_t class_id = pci_get_class_id(bus, device, function);
    uint8_t subclass_id = pci_get_subclass_id(bus, device, function);
    uint16_t vendor_id = pci_get_vendor_id(bus, device, function);
    uint16_t device_id = pci_read(bus, device, function, PCI_HEADER_0x0_DEVICE_ID);

    kprintf(LOG_INFO, RGB_COLOR_LIGHT_GRAY, "Found PCI device: %CBus %hhu Dev %hhu Funct %hhu%C Device ID: %C%hx%C Vendor ID: %C%hx%C Class ID: %C%hhu%C Subclass ID: %C%hhu%C\n",
        default_rgb_color, bus, device, function, RGB_COLOR_LIGHT_GRAY, 
        default_rgb_color, device_id, RGB_COLOR_LIGHT_GRAY,
        default_rgb_color, vendor_id, RGB_COLOR_LIGHT_GRAY,
        default_rgb_color, class_id, RGB_COLOR_LIGHT_GRAY,
        default_rgb_color, subclass_id, RGB_COLOR_LIGHT_GRAY);
    
    pci_devices.add({bus, device, function, vendor_id, device_id, class_id, subclass_id});
}

/// @brief Checks for a specific device
void pci::pci_check_device(uint8_t bus, uint8_t device) {
    // Not exists if invalid vendor
    if(pci::pci_get_vendor_id(bus, device, 0) == PCI_NO_DEVICE_VENDOR) return;

    // If we made it here the device exists, checking function 0
    pci::pci_manage_function(bus, device, 0);

    // Check header type to see if it is a Multifunction Device (Bit 7 set)
    uint8_t header_type = pci_get_header_type(bus, device, 0);

    if((header_type & 0x80) != 0) {
        // It is a multifunction device, so check the remaining 7 functions
        for(uint8_t func = 1; func < 8; func++) {
            if(pci_get_vendor_id(bus, device, func) != PCI_NO_DEVICE_VENDOR) {
                pci::pci_manage_function(bus, device, func);
            }
        }
    }
}

/// @brief Scans every bus for devices using the brute force technique
void pci::pci_brute_force_scan(void) {
    for(uint16_t bus = 0; bus < 256; bus++) {
        for(uint8_t device = 0; device < 32; device++) {
            pci::pci_check_device(bus, device);
        }
    }
}
