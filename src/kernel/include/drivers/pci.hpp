// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#ifndef PCI_HPP
#define PCI_HPP

#include <stdint.h>

// Configuration Space Access Mechanism #1 I/O locations
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_NO_DEVICE_VENDOR 0xFFFF
#define PCI_MULTI_FUNC_DEVICE_HEADER 0x80

// Class Codes
#define PCI_CLASS_UNCLASSIFIED       0x00
#define PCI_CLASS_MASS_STORAGE       0x01
#define PCI_CLASS_NETWORK            0x02
#define PCI_CLASS_DISPLAY            0x03
#define PCI_CLASS_MULTIMEDIA         0x04
#define PCI_CLASS_MEMORY             0x05
#define PCI_CLASS_BRIDGE             0x06
#define PCI_CLASS_COMMUNICATION      0x07
#define PCI_CLASS_BASE_SYSTEM        0x08
#define PCI_CLASS_INPUT              0x09
#define PCI_CLASS_DOCKING            0x0A
#define PCI_CLASS_PROCESSOR          0x0B
#define PCI_CLASS_SERIAL_BUS         0x0C
#define PCI_CLASS_WIRELESS           0x0D
#define PCI_CLASS_INTELLIGENT        0x0E
#define PCI_CLASS_SATELLITE          0x0F
#define PCI_CLASS_ENCRYPTION         0x10
#define PCI_CLASS_SIGNAL             0x11
#define PCI_CLASS_ACCELERATOR        0x12
#define PCI_CLASS_NON_ESSENTIAL      0x13
#define PCI_CLASS_RESERVED_0         0x14
#define PCI_CLASS_CO_PROCESSOR       0x40
#define PCI_CLASS_RESERVED_1         0x41
#define PCI_CLASS_UNASSIGNED         0xFF

// Mass Storage Controller subclasses
#define PCI_STORAGE_SCSI         0x0
#define PCI_STORAGE_IDE          0x1
#define PCI_STORAGE_FLOPPY       0x2
#define PCI_STORAGE_IPI          0x3
#define PCI_STORAGE_RAID         0x4
#define PCI_STORAGE_ATA          0x5
#define PCI_STORAGE_SATA         0x6
#define PCI_STORAGE_SERIAL_SCSI  0x7
#define PCI_STORAGE_NON_VOLATILE 0x8

#define PCI_SUBCLASS_OTHER        0x80

// Offsets in PCI Configuration Header (0x0)
#define PCI_HEADER_0x0_VENDOR_ID           0x00
#define PCI_HEADER_0x0_DEVICE_ID           0x02
#define PCI_HEADER_0x0_COMMAND             0x04
#define PCI_HEADER_0x0_STATUS              0x06
#define PCI_HEADER_0x0_REVISION_ID         0x08
#define PCI_HEADER_0x0_PROG_IF             0x09
#define PCI_HEADER_0x0_SUBCLASS            0x0A
#define PCI_HEADER_0x0_CLASS               0x0B
#define PCI_HEADER_0x0_CACHE_LINE_SIZE     0x0C
#define PCI_HEADER_0x0_LATENCY_TIMER       0x0D
#define PCI_HEADER_0x0_HEADER_TYPE         0x0E
#define PCI_HEADER_0x0_BIST                0x0F
#define PCI_HEADER_0x0_BAR0                0x10
#define PCI_HEADER_0x0_BAR1                0x14
#define PCI_HEADER_0x0_BAR2                0x18
#define PCI_HEADER_0x0_BAR3                0x1C
#define PCI_HEADER_0x0_BAR4                0x20
#define PCI_HEADER_0x0_BAR5                0x24
#define PCI_HEADER_0x0_CIS                 0x28
#define PCI_HEADER_0x0_SUBSYSTEM_VENDOR_ID 0x2C
#define PCI_HEADER_0x0_SUBSYSTEM_ID        0x2E
#define PCI_HEADER_0x0_EXPANSION_ROM       0x30
#define PCI_HEADER_0x0_CAPABILITIES        0x34
#define PCI_HEADER_0x0_RESERVED_0          0x35
#define PCI_HEADER_0x0_RESERVED_1          0x38
#define PCI_HEADER_0x0_INTERRUPT_LINE      0x3C
#define PCI_HEADER_0x0_INTERRUPT_PIN       0x3D
#define PCI_HEADER_0x0_MIN_GRANT           0x3E
#define PCI_HEADER_0x0_MAX_LATENCY         0x3F


class PciDevice {
private:
    const uint8_t bus;
    const uint8_t device;
    const uint8_t function;
    const uint16_t vendor_id;
    const uint16_t device_id;
    const uint8_t class_id;
    const uint8_t subclass_id;
    const uint8_t prog_if;
public:
    PciDevice(uint8_t bus, uint8_t device, uint8_t function, 
        uint16_t vendor_id, uint16_t device_id,
        uint8_t class_id, uint8_t subclass_id, uint8_t prog_if) :
            bus(bus), device(device), function(function), vendor_id(vendor_id), device_id(device_id),
            class_id(class_id), subclass_id(subclass_id), prog_if(prog_if) {}

    uint8_t get_bus() const;
    uint8_t get_device() const;
    uint8_t get_function() const;
    uint16_t get_vendor_id() const;
    uint16_t get_device_id() const;
    uint8_t get_class_id() const;
    uint8_t get_subclass_id() const;
    uint8_t get_prog_if() const;

    uint16_t read_word(uint8_t offset);
    void write_word(uint8_t offset, uint16_t data);
    uint32_t get_bar(uint8_t bar);

    void log_pci_info();
};

namespace pci {
    // PCI_HEADER_0x0_COMMAND
    union CommandRegister {
        uint16_t raw;
        struct {
            uint8_t io_space : 1;
            uint8_t mem_space : 1;
            uint8_t bus_master : 1;
            uint8_t spec_cycles : 1;
            uint8_t mem_write : 1;
            uint8_t vga_palette_snoop : 1;
            uint8_t parity_error_resp : 1;
            uint8_t reserved_0 : 1;
            uint8_t serr_enabled : 1;
            uint8_t fast_enable : 1;
            uint8_t interrupt_disable : 1;
            uint8_t reserved_1 : 5;
        } __attribute__((packed));
    };

    uint8_t pci_read8(PciDevice pci, uint8_t offset);
    uint8_t pci_read8(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
    uint16_t pci_read(PciDevice pci, uint8_t register_offset);
    uint16_t pci_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t register_offset);
    uint32_t pci_read32(PciDevice pci, uint8_t register_offset);
    uint32_t pci_read32(uint8_t bus, uint8_t device, uint8_t func, uint8_t register_offset);
    void pci_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t register_offset, uint32_t value);

    uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t function);
    uint8_t pci_get_header_type(uint8_t bus, uint8_t device, uint8_t func);
    uint8_t pci_get_class_id(uint8_t bus, uint8_t device, uint8_t func);
    uint8_t pci_get_subclass_id(uint8_t bus, uint8_t device, uint8_t func);
    uint8_t pci_get_prog_if(uint8_t bus, uint8_t device, uint8_t func);

    void pci_check_device(uint8_t bus, uint8_t device);
    void pci_manage_function(uint8_t bus, uint8_t device, uint8_t function);
    void pci_brute_force_scan(void);
} // namespace pci

#endif // PCI_HPP
