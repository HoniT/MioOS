// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef PMM_HPP
#define PMM_HPP

#include <stdint.h>
#include <kernel_main.hpp>

#define METADATA_ADDR 0x600000 // 6MiB mark
#define FRAME_SIZE 0x1000 // 4KiB frames

// Memory map entry structure
struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed));

// Linked list node for memory metadata
struct MetadataNode {
    uint64_t addr;
    uint64_t size;
    bool free : 1;
    MetadataNode* next;
    MetadataNode* prev;
} __attribute__((packed));

namespace pmm {
    // Variables
    extern uint64_t total_usable_ram;
    extern uint64_t total_used_ram;
    extern uint64_t hardware_reserved_ram;
    extern uint64_t total_installed_ram;

    // Linked list heads
    extern MetadataNode* low_alloc_mem_head;
    extern MetadataNode* high_alloc_mem_head;

    // Prints out memory map
    void print_memory_map(void);
    // Prints info of blocks in the allocatable memory regions 
    void print_usable_regions(void);
    // Gets the amount of usable RAM in the system
    void manage_mmap(struct multiboot_info* _mb_info);
    // Initializes any additional info for the PMM
    void init(struct multiboot_info* _mb_info);

    // Frame alloc / dealloc functions

    // Allocates a frame in the usable memory regions
    void* alloc_frame(const uint64_t num_blocks, bool identity_map = true);
    void free_frame(void* ptr);
} // Namespace pmm

#endif // PMM_HPP
