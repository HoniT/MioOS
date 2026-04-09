// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef PMM_HPP
#define PMM_HPP

#include <stdint.h>
#include <stddef.h>

namespace mem
{
    constexpr uint64_t PAGE_SIZE = 4096;
    constexpr uint64_t HIGHER_HALF_OFFSET = 0xFFFFFFFF80000000;

    class PMM {
    public:
        static void init(void* mbi);

        static void* alloc_frame();
        static void* alloc_frames(uint64_t count);

        static void free_frame(void* phys_addr);
        static void free_frames(void* phys_addr, uint64_t count);

        // Statistics
        static uint64_t get_total_memory();
        static uint64_t get_free_memory();
        static uint64_t get_used_memory();
    private:
        static void mark_region_free(uint64_t base, uint64_t length);
        static void mark_region_used(uint64_t base, uint64_t length);
        
        static void set_bit(size_t bit);
        static void clear_bit(size_t bit);
        static bool test_bit(size_t bit);

        // SMP Lock
        static void lock();
        static void unlock();

        static uint64_t* bitmap;
        static size_t bitmap_size_bytes;
        static uint64_t total_frames;
        static uint64_t free_frames_count;
        static uint64_t used_frames_count;
        static uint64_t last_scanned_index; // Optimization for fast allocation
    };
} // namespace pmm


#endif // PMM_HPP
