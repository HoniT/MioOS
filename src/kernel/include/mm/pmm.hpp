// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef PMM_HPP
#define PMM_HPP

#include <stdint.h>
#include <stddef.h>
#include <kernel_main.hpp>

#define DATA_LOW_START_ADDR 0x500000 // 5MiB mark
#define FRAME_SIZE 0x1000 // 4KiB frames

// Memory map entry structure
struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed));

// Linked list node for memory
struct MemoryNode {
    uint64_t size;
    bool free;
    MemoryNode* next;
};

namespace pmm {
    // Variables
    extern uint64_t total_usable_ram;
    extern uint64_t total_used_ram;

    // Prints out memory map
    void print_memory_map(void);
    // Gets the amount of usable RAM in the system
    void manage_mmap(struct multiboot_info* _mb_info);
    // Initializes any additional info for the PMM
    void init(struct multiboot_info* _mb_info);

    // Frame alloc / dealloc functions

    // Allocates a frame in the low usable memory region
    void* alloc_lframe(const size_t num_blocks);
    // Allocates a frame in the high usable memory region
    void* alloc_hframe(const size_t num_blocks);
    void free_frame(const void* ptr);
} // Namespace pmm

#endif // PMM_HPP