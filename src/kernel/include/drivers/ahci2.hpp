// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#ifndef AHCI2_HPP
#define AHCI2_HPP

#include <stdint.h>
#include <drivers/pci.hpp>

// Global Host Control (GHC) Bits
#define GHC_AE   (1 << 31)  // AHCI Enable
#define GHC_HR   (1 << 0)   // HBA Reset
#define GHC_IE   (1 << 1)   // Interrupt Enable

// Port Command (PxCMD) Bits
#define PxCMD_ST    (1 << 0)  // Start
#define PxCMD_FRE   (1 << 4)  // FIS Receive Enable
#define PxCMD_FR    (1 << 14) // FIS Receive Running
#define PxCMD_CR    (1 << 15) // Command List Running

// Port Interrupt Status
#define IS_IPS(x)   (1 << x)

typedef struct {
    uint32_t clb;       // 0x00 Command List Base Address
    uint32_t clbu;      // 0x04 Command List Base Upper
    uint32_t fb;        // 0x08 FIS Base Address
    uint32_t fbu;       // 0x0C FIS Base Upper
    uint32_t is;        // 0x10 Interrupt Status
    uint32_t ie;        // 0x14 Interrupt Enable
    uint32_t cmd;       // 0x18 Command and Status
    uint32_t reserved;  // 0x1C
    uint32_t tfd;       // 0x20 Task File Data
    uint32_t sig;       // 0x24 Signature
    uint32_t ssts;      // 0x28 SATA Status
    uint32_t sctl;      // 0x2C SATA Control
    uint32_t serr;      // 0x30 SATA Error
    uint32_t sact;      // 0x34 SATA Active
    uint32_t ci;        // 0x38 Command Issue
    uint32_t sntf;      // 0x3C SATA Notification
    uint32_t fbs;       // 0x40 FIS-based Switching
    uint32_t rsvd[11];
    uint32_t vendor[4];
} HBA_PORT;

typedef struct {
    uint32_t cap;       // 0x00 Host Capabilities
    uint32_t ghc;       // 0x04 Global Host Control
    uint32_t is;        // 0x08 Interrupt Status
    uint32_t pi;        // 0x0C Ports Implemented
    uint32_t vs;        // 0x10 Version
    uint32_t ccc_ctl;   // 0x14 Command Completion Coalescing Control
    uint32_t ccc_pts;   // 0x18 Command Completion Coalescing Ports
    uint32_t em_loc;    // 0x1C Enclosure Management Location
    uint32_t em_ctl;    // 0x20 Enclosure Management Control
    uint32_t cap2;      // 0x24 Host Capabilities Extended
    uint32_t bohc;      // 0x28 BIOS/OS Handoff Control and Status
    uint8_t  rsvd[0xA0-0x2C];
    uint8_t  vendor[0x100-0xA0];
    HBA_PORT ports[32]; // 0x100 - 0x10FF
} HBA_MEM;

class AhciDriver2 {
private:
    PciDevice* pci_dev;
    HBA_MEM* hba;
public:
    static void init_dev(PciDevice* pci_dev);
    bool port_reset(HBA_PORT* port);
    bool hba_reset();
};

#endif // AHCI2_HPP
