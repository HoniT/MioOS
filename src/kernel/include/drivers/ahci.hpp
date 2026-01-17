// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#ifndef AHCI_HPP
#define AHCI_HPP

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

// ATA Commands
#define ATA_CMD_READ_DMA_EX     0x25
#define ATA_CMD_WRITE_DMA_EX    0x35
#define ATA_CMD_IDENTIFY        0xEC

// ATA Status
#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ  0x08

// SATA Signatures
#define SATA_SIG_ATA    0x00000101  // SATA drive
#define SATA_SIG_ATAPI  0xEB140101  // SATAPI drive
#define SATA_SIG_SEMB   0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM     0x96690101  // Port multiplier



volatile typedef struct tagHBA_PORT{
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
} __attribute__((packed)) HBA_PORT;

volatile typedef struct {
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
} __attribute__((packed)) HBA_MEM;

typedef struct
{
    // DW0
    uint8_t  cfl:5;     // Command FIS length in DWORDS, 2 ~ 16
    uint8_t  a:1;       // ATAPI
    uint8_t  w:1;       // Write, 1: H2D, 0: D2H
    uint8_t  p:1;       // Prefetchable

    uint8_t  r:1;       // Reset
    uint8_t  b:1;       // BIST
    uint8_t  c:1;       // Clear busy upon R_OK
    uint8_t  rsv0:1;        // Reserved
    uint8_t  pmp:4;     // Port multiplier port

    uint16_t prdtl;     // Physical region descriptor table length in entries

    // DW1
    volatile uint32_t prdbc; // Physical region descriptor byte count transferred

    // DW2, 3
    uint32_t ctba;      // Command table descriptor base address
    uint32_t ctbau;     // Command table descriptor base address upper 32 bits

    // DW4 - 7
    uint32_t rsv1[4];   // Reserved
} __attribute__((packed)) HBA_CMD_HEADER;

typedef struct
{
    uint32_t dba;       // Data base address
    uint32_t dbau;      // Data base address upper 32 bits
    uint32_t rsv0;      // Reserved

    // DW3
    uint32_t dbc:22;        // Byte count, 4M max
    uint32_t rsv1:9;        // Reserved
    uint32_t i:1;       // Interrupt on completion
} __attribute__((packed)) HBA_PRDT_ENTRY;

typedef struct
{
    // 0x00
    uint8_t  cfis[64];  // Command FIS

    // 0x40
    uint8_t  acmd[16];  // ATAPI command, 12 or 16 bytes

    // 0x50
    uint8_t  rsv[48];   // Reserved

    // 0x80
    HBA_PRDT_ENTRY  prdt_entry[1];  // Physical region descriptor table entries, 0 ~ 65535
} __attribute__((packed)) HBA_CMD_TBL;

typedef struct {
    uint16_t general_config;        // Word 0: General Configuration
    uint16_t unused1[9];            // Words 1-9
    char     serial[20];            // Words 10-19: Serial Number
    uint16_t unused2[3];            // Words 20-22
    char     firmware[8];           // Words 23-26: Firmware Revision
    char     model[40];             // Words 27-46: Model Number
    uint16_t sectors_per_int;       // Word 47: Max sectors per interrupt
    uint16_t unused3;               // Word 48
    uint16_t capabilities[2];       // Words 49-50: Capabilities
    uint16_t unused4[2];            // Words 51-52
    uint16_t valid_fields;          // Word 53: Validity field
    uint16_t unused5[5];            // Words 54-58
    uint16_t multi_sector;          // Word 59: Multi-sector setting
    uint32_t lba28_sectors;         // Words 60-61: Total sectors (LBA28)
    uint16_t unused6[38];           // Words 62-99
    uint64_t lba48_sectors;         // Words 100-103: Total sectors (LBA48)
    uint16_t unused7[152];          // Words 104-255
} __attribute__((packed)) SATA_IDENTIFY_DATA;

#pragma region FIS

enum FIS_TYPE
{
    FIS_TYPE_REG_H2D    = 0x27, // Register FIS - host to device
    FIS_TYPE_REG_D2H    = 0x34, // Register FIS - device to host
    FIS_TYPE_DMA_ACT    = 0x39, // DMA activate FIS - device to host
    FIS_TYPE_DMA_SETUP  = 0x41, // DMA setup FIS - bidirectional
    FIS_TYPE_DATA       = 0x46, // Data FIS - bidirectional
    FIS_TYPE_BIST       = 0x58, // BIST activate FIS - bidirectional
    FIS_TYPE_PIO_SETUP  = 0x5F, // PIO setup FIS - device to host
    FIS_TYPE_DEV_BITS   = 0xA1, // Set device bits FIS - device to host
};

struct FIS_REG_H2D
{
    // DWORD 0
    uint8_t  fis_type;  // FIS_TYPE_REG_H2D

    uint8_t  pmport:4;  // Port multiplier
    uint8_t  rsv0:3;        // Reserved
    uint8_t  c:1;       // 1: Command, 0: Control

    uint8_t  command;   // Command register
    uint8_t  featurel;  // Feature register, 7:0
    
    // DWORD 1
    uint8_t  lba0;      // LBA low register, 7:0
    uint8_t  lba1;      // LBA mid register, 15:8
    uint8_t  lba2;      // LBA high register, 23:16
    uint8_t  device;        // Device register

    // DWORD 2
    uint8_t  lba3;      // LBA register, 31:24
    uint8_t  lba4;      // LBA register, 39:32
    uint8_t  lba5;      // LBA register, 47:40
    uint8_t  featureh;  // Feature register, 15:8

    // DWORD 3
    uint8_t  countl;        // Count register, 7:0
    uint8_t  counth;        // Count register, 15:8
    uint8_t  icc;       // Isochronous command completion
    uint8_t  control;   // Control register

    // DWORD 4
    uint8_t  rsv1[4];   // Reserved
} __attribute__((packed));

struct FIS_REG_D2H
{
    // DWORD 0
    uint8_t  fis_type;    // FIS_TYPE_REG_D2H

    uint8_t  pmport:4;    // Port multiplier
    uint8_t  rsv0:2;      // Reserved
    uint8_t  i:1;         // Interrupt bit
    uint8_t  rsv1:1;      // Reserved

    uint8_t  status;      // Status register
    uint8_t  error;       // Error register

    // DWORD 1
    uint8_t  lba0;        // LBA low register, 7:0
    uint8_t  lba1;        // LBA mid register, 15:8
    uint8_t  lba2;        // LBA high register, 23:16
    uint8_t  device;      // Device register

    // DWORD 2
    uint8_t  lba3;        // LBA register, 31:24
    uint8_t  lba4;        // LBA register, 39:32
    uint8_t  lba5;        // LBA register, 47:40
    uint8_t  rsv2;        // Reserved

    // DWORD 3
    uint8_t  countl;      // Count register, 7:0
    uint8_t  counth;      // Count register, 15:8
    uint8_t  rsv3[2];     // Reserved

    // DWORD 4
    uint8_t  rsv4[4];     // Reserved
} __attribute__((packed));

struct FIS_DATA
{
    // DWORD 0
    uint8_t  fis_type;  // FIS_TYPE_DATA

    uint8_t  pmport:4;  // Port multiplier
    uint8_t  rsv0:4;        // Reserved

    uint8_t  rsv1[2];   // Reserved

    // DWORD 1 ~ N
    uint32_t data[1];   // Payload
} __attribute__((packed));

struct FIS_PIO_SETUP
{
    // DWORD 0
    uint8_t  fis_type;  // FIS_TYPE_PIO_SETUP

    uint8_t  pmport:4;  // Port multiplier
    uint8_t  rsv0:1;        // Reserved
    uint8_t  d:1;       // Data transfer direction, 1 - device to host
    uint8_t  i:1;       // Interrupt bit
    uint8_t  rsv1:1;

    uint8_t  status;        // Status register
    uint8_t  error;     // Error register

    // DWORD 1
    uint8_t  lba0;      // LBA low register, 7:0
    uint8_t  lba1;      // LBA mid register, 15:8
    uint8_t  lba2;      // LBA high register, 23:16
    uint8_t  device;        // Device register

    // DWORD 2
    uint8_t  lba3;      // LBA register, 31:24
    uint8_t  lba4;      // LBA register, 39:32
    uint8_t  lba5;      // LBA register, 47:40
    uint8_t  rsv2;      // Reserved

    // DWORD 3
    uint8_t  countl;        // Count register, 7:0
    uint8_t  counth;        // Count register, 15:8
    uint8_t  rsv3;      // Reserved
    uint8_t  e_status;  // New value of status register

    // DWORD 4
    uint16_t tc;        // Transfer count
    uint8_t  rsv4[2];   // Reserved
} __attribute__((packed));

struct FIS_DMA_SETUP
{
	// DWORD 0
	uint8_t  fis_type;
	uint8_t  pmport:4;
	uint8_t  rsv0:1;
	uint8_t  d:1;
	uint8_t  i:1;
	uint8_t  a:1;

    uint8_t  rsved[2];       // Reserved

    //DWORD 1&2

    uint64_t DMAbufferID;    // DMA Buffer Identifier. Used to Identify DMA buffer in host memory.
                                // SATA Spec says host specific and not in Spec. Trying AHCI spec might work.

    //DWORD 3
    uint32_t rsvd;           //More reserved

    //DWORD 4
    uint32_t DMAbufOffset;   //Byte offset into buffer. First 2 bits must be 0

    //DWORD 5
    uint32_t TransferCount;  //Number of bytes to transfer. Bit 0 must be 0

    //DWORD 6
    uint32_t resvd;          //Reserved
} __attribute__((packed));

struct FIS_DEV_BITS {
    // DWORD 0
    uint8_t  fis_type;        // Value is always 0xA1 (FIS_TYPE_DEV_BITS)
    
    uint8_t  pmport : 4;     // Port Multiplier Port
    uint8_t  rsv0 : 2;  // Reserved
    uint8_t  interrupt : 1;  // 'I' Bit: Interrupt flag
    uint8_t  notification : 1; // 'N' Bit: Notification flag
    
    uint8_t  status;      // Status Register (Bits 0-7)
    uint8_t  error;          // Error Register
    
    // DWORD 1
    uint32_t protocol; // Protocol Specific (often SActive/NCQ)
    
} __attribute__((packed));

volatile typedef struct
{
    // 0x00
    FIS_DMA_SETUP   dsfis;      // DMA Setup FIS
    uint8_t         pad0[4];

    // 0x20
    FIS_PIO_SETUP   psfis;      // PIO Setup FIS
    uint8_t         pad1[12];

    // 0x40
    FIS_REG_D2H rfis;       // Register – Device to Host FIS
    uint8_t         pad2[4];

    // 0x58
    FIS_DEV_BITS    sdbfis;     // Set Device Bit FIS
    
    // 0x60
    uint8_t         ufis[64];

    // 0xA0
    uint8_t     rsv[0x100-0xA0];
} __attribute__((packed)) HBA_FIS;

#pragma endregion

class AhciDriver {
private:
    PciDevice* pci_dev;
    HBA_MEM* hba;

    void probe_ports();
    void configure_drive(HBA_PORT* port);

public:
    static void init_dev(PciDevice* pci_dev);

    bool port_reset(HBA_PORT* port);
    bool hba_reset();

    bool start_cmd(HBA_PORT* port);
    bool stop_cmd(HBA_PORT* port);

    bool identify(HBA_PORT* port, SATA_IDENTIFY_DATA* buffer);
    bool read(HBA_PORT* port, uint64_t sector, uint32_t count, void* buffer);
    bool write(HBA_PORT* port, uint64_t sector, uint32_t count, void* buffer);

    static bool check_connection(HBA_PORT* port);
    static bool wait_bit_clear(uint32_t reg, uint32_t mask, int timeout_ms);
    int8_t find_cmdslot(HBA_PORT *port);

	PciDevice* get_pci_dev();
};

#endif // AHCI_HPP
