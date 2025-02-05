// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VMM_HPP
#define VMM_HPP


#include <stdint.h>

extern bool enabled_paging;

// Needed macros and macro functions
#define PD_ENTRIES 1024
#define PT_ENTRIES 1024
#define PAGE_SIZE 4096

#define KERNEL_VIRT_BASE 0xC0000000

#define PD_INDEX(address) (((address) >> 22) & 0x3FF)
#define PT_INDEX(address) (((address) >> 12) & 0x3FF) // Max index 1023 = 0x3FF
#define PAGE_PHYS_ADDRESS(dir_entry) ((*dir_entry) & ~0xFFF)    // Clear lowest 12 bits, only return frame/address
#define SET_ATTRIBUTE(entry, attr) (*entry |= attr)
#define CLEAR_ATTRIBUTE(entry, attr) (*entry &= ~attr)
#define TEST_ATTRIBUTE(entry, attr) (*entry & attr)
#define SET_FRAME(entry, address) (*entry = (*entry & ~0x7FFFF000) | address)   // Only set address/frame, not flags

typedef uint32_t pt_ent; // Page table entry
typedef uint32_t pd_ent; // Page directory entry

struct InterruptFrame {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp; // This is the stack before the interrupt
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t interruptNumber; // Interrupt vector number
    uint32_t errorCode;       // Error code (only for some interrupts like Page Fault)
    uint32_t eip;             // Instruction pointer
    uint32_t cs;              // Code segment
    uint32_t eflags;          // Flags register
    uint32_t userEsp;         // Stack pointer (if switching to ring 3)
    uint32_t userSs;          // Stack segment (if switching to ring 3)
};


// PTE flags
enum PTE_FLAGS {
    PTE_PRESENT      = 0x1,
    PTE_WRITABLE     = 0x2,
    PTE_USER         = 0x4,
    PTE_WRITETHROUGH = 0x8,
    PTE_NOTCACHABLE  = 0x10,
    PTE_ACCESSED     = 0x20,
    PTE_DIRTY        = 0x40,
    PTE_PAT          = 0x80,   // Page Attribute Table
    PTE_CPU_GLOBAL   = 0x100,
    PTE_LV4_GLOBAL   = 0x200,
    PTE_FRAME        = 0x7FFFF000 
};

// PDE flags
enum PDE_FLAGS {
    PDE_PRESENT      = 0x1,
    PDE_WRITABLE     = 0x2,
    PDE_USER         = 0x4,
    PDE_WRITETHROUGH = 0x8,
    PDE_NOTCACHABLE  = 0x10,
    PDE_ACCESSED     = 0x20,
    PDE_DIRTY        = 0x40,
    PDE_PS           = 0x80,   // If set to 1 it will have a 4MiB page
    PDE_CPU_GLOBAL   = 0x100,
    PDE_LV4_GLOBAL   = 0x200,
    PDE_FRAME        = 0x7FFFF000 
};

// Page Table: handle 4MiB (1024 entries * 4096)
struct PageTable {
    pt_ent entries[PT_ENTRIES];
} __attribute__((packed));

// Page Directory: handle 4GiB (1024 page tables * 4MiB)
struct PageDirectory {
    pd_ent entries[PD_ENTRIES];
} __attribute__((packed));

extern PageDirectory* current_page_directory;

namespace vmm {
    // Needed functions

    void flush_tlb(const void* virtAddr); // Flushes TLB
    void set_pd(PageDirectory* pageDirectory); // Sets the page directory
    void init(); // Initializes the VMM

    // Mapping and unmapping pages
    void map_page(const uint32_t virtAddr, const uint32_t physAddr, uint32_t flags);
    void unmap_page(const uint32_t virtAddr);
    void* allocate_page(pt_ent* page);
    void free_page(pt_ent* page);

    // Helper functions

    // Gets a page for a given virtual address in the current PD
    pt_ent* get_page(const uint32_t virtAddr);
    // Gets entry in a given PD for a given address
    pd_ent* get_pd_ent(const PageDirectory* pd, const uint32_t virtAddr);
    // Gets entry in a given PT for a given address
    pt_ent* get_pt_ent(const PageTable* pt, const uint32_t virtAddr);
    // Checks if the page at a given virtual address is mapped
    bool is_mapped(const void* virtAddr); 
    uint32_t translate_virtual_to_physical(const uint32_t virtAddr);

    void test_page_mapping(const void* virtAddr);

    void page_fault_handler(InterruptFrame* frame);

} // namespace vmm

// External functions from paging.asm

extern "C" void set_page_directory(PageDirectory*);
extern "C" void enable_paging();


#endif // VMM_HPP
