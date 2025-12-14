// =======================================================================
// Copyright Ioane Baidoshvili 2024.
// Distributed under the terms of the MIT License.
// =======================================================================

#pragma once

#ifndef VMM_HPP
#define VMM_HPP

#include <stdint.h>

#define PAGE_SIZE 4096
#define PD_ENTRIES 1024
#define PT_ENTRIES 1024

#define KERNEL_LOAD_ADDRESS 0xC0000000

#define PD_INDEX(vaddr)   (((vaddr) >> 22) & 0x3FF)
#define PT_INDEX(vaddr)   (((vaddr) >> 12) & 0x3FF)
#define PAGE_OFFSET(vaddr) ((vaddr) & 0xFFF)
#define FRAME_TO_PHYS(vaddr) (vaddr << 12)
#define PHYS_TO_FRAME(vaddr) (vaddr >> 12)
#define FRAME4MB_TO_PHYS(vaddr) (vaddr << 22)
#define PHYS_TO_FRAME4MB(vaddr) (vaddr >> 22)

#pragma region Paging Structures

// Entries for PD and PT

struct pd_ent {
    uint32_t present : 1;
    uint32_t read_write : 1;
    uint32_t user_supervisor : 1;
    uint32_t write_through : 1;
    uint32_t cache_disable : 1;
    uint32_t accessed : 1;
    uint32_t ignored_0 : 1;
    uint32_t ps : 1;
    uint32_t ignored_1 : 4;
    uint32_t address : 20;
} __attribute__((packed));

struct page_4mb {
    uint32_t present : 1;
    uint32_t read_write : 1;
    uint32_t user_supervisor : 1;
    uint32_t write_through : 1;
    uint32_t cache_disable : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t ps : 1;
    uint32_t global : 1;
    uint32_t ignored : 3;
    uint32_t pat : 1;
    uint32_t address : 19;
} __attribute__((packed));

struct page_4kb {
    uint32_t present : 1;
    uint32_t read_write : 1;
    uint32_t user_supervisor : 1;
    uint32_t write_through : 1;
    uint32_t cache_disable : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t pat : 1;
    uint32_t global : 1;
    uint32_t ignored : 3;
    uint32_t address : 20;
} __attribute__((packed));

// PD and PT

struct pt_t {
    page_4kb pages[PT_ENTRIES];
};

struct pd_t {
    pd_ent entries[PD_ENTRIES];
    pt_t* page_tables[PD_ENTRIES];
};

enum PAGING_FLAGS {
    PRESENT      = 0x1,
    WRITABLE     = 0x2,
    USER         = 0x4,
    WRITETHROUGH = 0x8,
    NOTCACHABLE  = 0x10,
    ACCESSED     = 0x20,
    DIRTY        = 0x40,
    PAT          = 0x80,
    PS           = 0x80,
    CPU_GLOBAL   = 0x100
};

#pragma endregion

namespace vmm {
    extern bool enabled_paging;
    extern bool pae_paging;

    pd_t* get_active_pd(void);

    // Initializes the VMM
    void init(void);
    // Allocates a 4 KiB page
    void alloc_page(const uint32_t virt_addr, const uint32_t phys_addr, const uint32_t flags);
    // Allocates a 4 MiB page
    void alloc_page_4mib(const uint32_t virt_addr, const uint32_t phys_addr, const uint32_t flags);
    // Identity maps a region in a given range
    void identity_map_region(const uint32_t start_addr, const uint32_t end_addr, const uint32_t flags);
    // Frees a page at a give virtual address
    bool free_page(const uint32_t virt_addr);
    // Returns the corresponding physical address for a virtual address
    void* virtual_to_physical(const uint32_t virt_addr);
    // Returns if a page at the given virtual address is mapped or not
    bool is_mapped(const uint32_t virt_addr);
}

// Functions defined in ASM
extern "C" void set_pd(uint32_t);
extern "C" void enable_paging(void);
extern "C" void reload_cr3(void);
extern "C" void invlpg(uint32_t);

#endif // VMM_HPP
