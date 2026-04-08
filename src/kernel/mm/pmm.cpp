// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <mm/pmm.hpp>
#include <multiboot.hpp>
#include <hal/cpu.hpp>

extern "C" uint8_t kernel_start_phys[];
extern "C" uint8_t kernel_end_phys[];

namespace mem
{
    // Initialize static members
    uint64_t* PMM::bitmap = nullptr;
    size_t PMM::bitmap_size_bytes = 0;
    size_t PMM::total_frames = 0;
    size_t PMM::free_frames_count = 0;
    size_t PMM::used_frames_count = 0;
    size_t PMM::last_scanned_index = 0;

    void PMM::set_bit(size_t bit) {
        bitmap[bit / 64] |= (1ULL << (bit % 64));
    }

    void PMM::clear_bit(size_t bit) {
        bitmap[bit / 64] &= ~(1ULL << (bit % 64));
    }

    bool PMM::test_bit(size_t bit) {
        return bitmap[bit / 64] & (1ULL << (bit % 64));
    }

    void PMM::lock() { /* TODO: Acquire atomic spinlock */ }
    void PMM::unlock() { /* TODO: Release atomic spinlock */ }

    /// @brief Initializes the PMM
    /// @param mbi Multiboot2 info
    void PMM::init(void* mbi) {
        multiboot_tag_mmap* mmap_tag = Multiboot2::get_mmap(mbi);
        if (!mmap_tag) {
            hal::cpu::halt();
            return;
        }

        
    }

    size_t PMM::get_total_memory() { return total_frames * PAGE_SIZE; }
    size_t PMM::get_free_memory() { return free_frames_count * PAGE_SIZE; }
    size_t PMM::get_used_memory() { return used_frames_count * PAGE_SIZE; }
} // namespace mem
