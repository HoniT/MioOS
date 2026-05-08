// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef PMM_HPP
#define PMM_HPP

#include <mm/mm_defs.hpp>
#include <stddef.h>

namespace mem
{
    constexpr PhysAddr FRAME_SIZE = 4096;

    class PMM {
    public:
        /// @brief Initializes the PMM
        /// @param mbi Multiboot2 info
        static void init(void* mbi);

        /// @brief Allocates a single physical frame
        /// @return Allocated frame physical address
        static void* alloc_frame();
        /// @brief Allocates multiple frames
        /// @param count Number of frames to allocate
        /// @return First frames physical address
        static void* alloc_frames(size_t count);

        /// @brief Frees a frame
        /// @param phys_addr Physical address of the frame
        static void free_frame(void* phys_addr);
        /// @brief Frees multiple frames
        /// @param phys_addr Physical address of first frame
        /// @param count Number of frames to free
        static void free_frames(void* phys_addr, size_t count);

        /// @brief Marks a physical memore region as free
        /// @param base Physical address of the regions start
        /// @param length Length of the region
        static void mark_region_free(PhysAddr base, size_t length);
        /// @brief Marks a physical memore region as allocated
        /// @param base Physical address of the regions start
        /// @param length Length of the region
        static void mark_region_used(PhysAddr base, size_t length);

        // Statistics
        static size_t get_total_memory();
        static size_t get_free_memory();
        static size_t get_used_memory();
    private:
        // Bitmap helper methods
        static void set_bit(uint64_t bit);
        static void clear_bit(uint64_t bit);
        static bool test_bit(uint64_t bit);

        // SMP Lock (TODO in far future)
        static void lock();
        static void unlock();

        static PhysAddr* bitmap;
        static size_t bitmap_size_bytes;
        static size_t total_frames;
        static size_t free_frames_count;
        static size_t used_frames_count;
        static uint64_t last_scanned_index; // Optimization for fast allocation
    }; // class PMM
} // namespace mem

#endif // PMM_HPP