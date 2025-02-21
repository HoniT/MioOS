// =======================================================================
// Copyright Ioane Baidoshvili 2024.
// Distributed under the terms of the MIT License.
// =======================================================================

#pragma once

#ifndef VMM_HPP
#define VMM_HPP

#include <stdint.h>

#define PAGE_SIZE 4096

extern bool enabled_paging;

#pragma region Paging Structures

// Macro functions for paging structures

// Sets an address for a page
#define SET_FRAME(entry, addr) (*(entry) = (*(entry) & ~FRAME) | ((addr) & FRAME))
// Gets the address of a page
#define GET_FRAME(entry) ((*(entry) & FRAME))
// Sets a flag for a page
#define SET_FLAG(entry, flag) (*(entry) |= (flag))
// Gets a flag of a page
#define GET_FLAG(entry, flag) (*(entry) & (flag))
// Gets indexes from addresses
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x3)  // Top 2 bits (30-31)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF) // Next 9 bits (21-29)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF) // Next 9 bits (12-20)
#define PAGE_OFFSET(addr) ((addr) & 0xFFF)  // Last 12 bits (0-11)

// Macros defining entries in each structure
#define PDPT_ENTRIES 4
#define PD_ENTRIES 512
#define PT_ENTRIES 512

// Entries
typedef uint64_t pdpt_ent; // Same as PD
typedef uint64_t pd_ent; // Same as PT
typedef uint64_t pt_ent; // Same as page

// Structures
typedef struct PDPT {
    pdpt_ent entries[PDPT_ENTRIES] __attribute__((aligned(32)));
} __attribute__((packed)) pdpt_t;

typedef struct PD {
    pd_ent entries[PD_ENTRIES] __attribute__((aligned(32)));
} __attribute__((packed)) pd_t;

typedef struct PT {
    pt_ent entries[PT_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
} __attribute__((packed)) pt_t;

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
    // Maps an address in virtual memory with given flags
    void map_page(const uint64_t virt_addr, const uint64_t phys_addr, const uint32_t flags);
    // Unmaps a page with the given virtual address
    void unmap_page(const uint64_t virt_addr);

    // Debug/helper function

    // Logs and returns a boolean value corresponding if a page at a given address is present/mapped
    bool is_mapped(const uint64_t virt_addr);
    // Returnes the physical address corresponding to a virtual address
    uint64_t get_physical_address(const uint64_t virt_addr);

} // namespace vmm

// ASM functions
extern "C" void set_pdpt(uint64_t);
extern "C" void enable_pae();
extern "C" void enable_paging();
extern "C" void flush_tlb(uint64_t);

#endif // VMM_HPP
