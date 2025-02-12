// =======================================================================
// Copyright Ioane Baidoshvili 2024.
// Distributed under the terms of the MIT License.
// =======================================================================

#pragma once

#ifndef VMM_HPP
#define VMM_HPP

#include <stdint.h>

#define PAGE_SIZE 4096
// Macros defining entries in each structure
#define PDPT_ENTRIES 4
#define PD_ENTRIES 512
#define PT_ENTRIES 512

// Macro functions for paging structures

// Sets an address for a page
#define SET_FRAME(entry, addr) ((entry)->frame = ((addr) >> 12))
// Gets the address of a page
#define GET_FRAME(entry) (((entry)->frame) << 12)
// Sets a flag for a page
#define SET_FLAG(entry, flag) (*(uint64_t *)(entry) |= (flag))
// Gets a flag of a page
#define GET_FLAG(entry, flag) ((entry)->flag)

extern bool enabled_paging;

#pragma region Structures and Enums

// Page Directory Pointer Table entry
typedef struct PDPTEntry {
    uint64_t present : 1;         // Bit 0: Present
    uint64_t rw : 1;              // Bit 1: Read/Write
    uint64_t user : 1;            // Bit 2: User/Supervisor
    uint64_t write_through : 1;   // Bit 3: Write-Through
    uint64_t cache_disable : 1;   // Bit 4: Cache Disable
    uint64_t accessed : 1;        // Bit 5: Accessed
    uint64_t dirty : 1;           // Bit 6: Dirty (if 1GB pages)
    uint64_t reserved1 : 1;       // Bit 7: Reserved (Must be 0)
    uint64_t global : 1;          // Bit 8: Global
    uint64_t avl : 3;             // Bits 9-11: Available for software use
    uint64_t pat : 1;             // Bit 12: Page Attribute Table
    uint64_t reserved2 : 17;      // Bits 13-29: Reserved (Must be 0)
    uint64_t frame : 22;          // Bits 30-51: Physical address of Page Directory
    uint64_t avl2 : 7;            // Bits 52-58: Available for software use
    uint64_t pk : 4;              // Bits 59-62: Protection Keys (Intel MPK)
    uint64_t xd : 1;              // Bit 63: Execute Disable
} __attribute__((packed)) pdpt_ent;

// Page Directory entry
typedef struct PDEntry {
    uint64_t present : 1;         // Bit 0: Present
    uint64_t rw : 1;              // Bit 1: Read/Write
    uint64_t user : 1;            // Bit 2: User/Supervisor
    uint64_t write_through : 1;   // Bit 3: Write-Through
    uint64_t cache_disable : 1;   // Bit 4: Cache Disable
    uint64_t accessed : 1;        // Bit 5: Accessed
    uint64_t dirty : 1;           // Bit 6: Dirty
    uint64_t ps : 1;              // Bit 7: Page Size (1 = 2MB page)
    uint64_t global : 1;          // Bit 8: Global
    uint64_t avl : 3;             // Bits 9-11: Available for software use
    uint64_t pat : 1;             // Bit 12: Page Attribute Table
    uint64_t reserved1 : 8;       // Bits 13-20: Reserved (Must be 0)
    uint64_t frame : 31;          // Bits 21-51: Physical address of 2MB page
    uint64_t avl2 : 7;            // Bits 52-58: Available for software use
    uint64_t pk : 4;              // Bits 59-62: Protection Keys (Intel MPK)
    uint64_t xd : 1;              // Bit 63: Execute Disable
} __attribute__((packed)) pd_ent;

// Page Table entry
typedef struct PTEntry {
    uint64_t present : 1;         // Bit 0: Present
    uint64_t rw : 1;              // Bit 1: Read/Write
    uint64_t user : 1;            // Bit 2: User/Supervisor
    uint64_t write_through : 1;   // Bit 3: Write-Through
    uint64_t cache_disable : 1;   // Bit 4: Cache Disable
    uint64_t accessed : 1;        // Bit 5: Accessed
    uint64_t dirty : 1;           // Bit 6: Dirty
    uint64_t pat : 1;             // Bit 7: PAT (4KB pages use Bit 7, not 12)
    uint64_t global : 1;          // Bit 8: Global
    uint64_t avl : 3;             // Bits 9-11: Available for software use
    uint64_t frame : 40;          // Bits 12-51: Physical address of 4KB page
    uint64_t avl2 : 7;            // Bits 52-58: Available for software use
    uint64_t pk : 4;              // Bits 59-62: Protection Keys (Intel MPK)
    uint64_t xd : 1;              // Bit 63: Execute Disable
} __attribute__((packed)) pt_ent;

// PT itself
typedef struct PT {
    pt_ent entries[PT_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
} __attribute__((packed)) pt_t;

// PD itself
typedef struct PD {
    pt_t* entries[PD_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
} __attribute__((packed)) pd_t;

// PDPT itself
typedef struct PDPT {
    pd_t* entries[PDPT_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
} __attribute__((packed)) pdpt_t;

// Paging structure flags
enum PAGING_FLAGS {
    PRESENT      = 0x1,
    WRITABLE     = 0x2,
    USER         = 0x4,
    WRITETHROUGH = 0x8,
    NOTCACHABLE  = 0x10,
    ACCESSED     = 0x20,
    DIRTY        = 0x40,
    PAT          = 0x80,   // Page Attribute Table
    CPU_GLOBAL   = 0x100,
    LV4_GLOBAL   = 0x200,
    FRAME        = 0x7FFFF000 
};

#pragma endregion

namespace vmm {
    // Initializes the VMM, sets PD, PDPT, PT structures
    void init(void);

} // namespace vmm

// ASM functions
extern "C" void set_pdpt(uint64_t);
extern "C" void enable_pae();
extern "C" void enable_paging();
extern "C" void flush_tlb(uint64_t);

#endif // VMM_HPP
