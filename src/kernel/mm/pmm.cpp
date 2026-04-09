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
    uint64_t PMM::total_frames = 0;
    uint64_t PMM::free_frames_count = 0;
    uint64_t PMM::used_frames_count = 0;
    uint64_t PMM::last_scanned_index = 0;

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

        uint32_t num_entries = (mmap_tag->size - sizeof(multiboot_tag_mmap)) / mmap_tag->entry_size;
        
        // Finding the highest address to scale the bitmap accordingly
        uint64_t top_physical_memory = 0;
        for (uint32_t i = 0; i < num_entries; i++) {
            multiboot_mmap_entry* entry = &mmap_tag->entries[i];
            uint64_t entry_end = entry->addr + entry->len;
            if (entry_end > top_physical_memory) {
                top_physical_memory = entry_end;
            }
        }

        total_frames = top_physical_memory / PAGE_SIZE;
        bitmap_size_bytes = (total_frames / 8) + 1;

        // Placing the bitmap immediately following the kernel
        uint64_t bitmap_phys_addr = (uint64_t)kernel_end_phys;
        bitmap = reinterpret_cast<uint64_t*>(bitmap_phys_addr + HIGHER_HALF_OFFSET);
    
        // Mark EVERYTHING as used by default
        for (size_t i = 0; i < bitmap_size_bytes / 8; i++) {
            bitmap[i] = ~0ULL; // 0xFFFFFFFFFFFFFFFF
        }
        used_frames_count = total_frames;
        free_frames_count = 0;

        // Mark available regions as free based on GRUB's map
        for (uint32_t i = 0; i < num_entries; i++) {
            multiboot_mmap_entry* entry = &mmap_tag->entries[i];
            // MULTIBOOT_MEMORY_AVAILABLE is type 1
            if (entry->type == 1) { 
                mark_region_free(entry->addr, entry->len);
            }
        }

        // Explicitly protect crucial physical memory regions
        
        // A. Kernel
        uint64_t kernel_size = (uint64_t)kernel_end_phys - (uint64_t)kernel_start_phys;
        mark_region_used((uint64_t)kernel_start_phys, kernel_size);

        // B. Bitmap
        mark_region_used(bitmap_phys_addr, bitmap_size_bytes);

        // C. Multiboot2 info structure
        uint64_t mb2_phys = (uint64_t)mbi - HIGHER_HALF_OFFSET;
        uint32_t mb2_size = *reinterpret_cast<uint32_t*>(mbi); // First 4 bytes are total size
        mark_region_used(mb2_phys, mb2_size);
    }


    void* PMM::alloc_frame() {
        lock();
        
        // Scan 64 frames at a time
        for (uint64_t i = last_scanned_index; i < total_frames / 64; i++) {
            // If the 64-bit block is not entirely 1s (not full)
            if (bitmap[i] != ~0ULL) {
                int bit = __builtin_ffsll(~bitmap[i]) - 1;
                
                uint64_t frame_index = (i * 64) + bit;
                set_bit(frame_index);
                
                last_scanned_index = i;
                free_frames_count--;
                used_frames_count++;
                
                unlock();
                return reinterpret_cast<void*>(frame_index * PAGE_SIZE);
            }
        }

        // Fallback: If we hit the end of the bitmap, loop back to the start once
        if (last_scanned_index > 0) {
            last_scanned_index = 0;
            unlock();
            return alloc_frame(); 
        }

        unlock();
        return nullptr; // Out of memory
    }

    void* PMM::alloc_frames(uint64_t count) {
        if (count == 0) return nullptr;
        
        lock();
        uint64_t start_frame = 0;
        uint64_t consecutive_free = 0;

        // Linear search for contiguous blocks (Needed for VMM tables or DMA)
        for (uint64_t i = 0; i < total_frames; i++) {
            if (!test_bit(i)) {
                if (consecutive_free == 0) start_frame = i;
                consecutive_free++;
                
                if (consecutive_free == count) {
                    for (size_t j = 0; j < count; j++) {
                        set_bit(start_frame + j);
                    }
                    free_frames_count -= count;
                    used_frames_count += count;
                    unlock();
                    return reinterpret_cast<void*>(start_frame * PAGE_SIZE);
                }
            } else {
                consecutive_free = 0; // Reset counter if we hit a used frame
            }
        }
        
        unlock();
        return nullptr;
    }

    void PMM::free_frame(void* phys_addr) {
        lock();
        uint64_t frame_index = reinterpret_cast<uint64_t>(phys_addr) / PAGE_SIZE;
        if (test_bit(frame_index)) {
            clear_bit(frame_index);
            free_frames_count++;
            used_frames_count--;
            
            // Move the scanner back to optimize the next allocation
            if (frame_index / 64 < last_scanned_index) {
                last_scanned_index = frame_index / 64;
            }
        }
        unlock();
    }

    void PMM::free_frames(void* phys_addr, uint64_t count) {
        uint64_t start_frame = reinterpret_cast<uint64_t>(phys_addr) / PAGE_SIZE;
        lock();
        for (uint64_t i = 0; i < count; i++) {
            if (test_bit(start_frame + i)) {
                clear_bit(start_frame + i);
                free_frames_count++;
                used_frames_count--;
            }
        }
        if (start_frame / 64 < last_scanned_index) {
            last_scanned_index = start_frame / 64;
        }
        unlock();
    }

    /// @brief Marks a specific region of memory in bitmap as free
    /// @param base Region base physical address
    /// @param length Region length
    void PMM::mark_region_free(uint64_t base, uint64_t length) {
        uint64_t align_offset = base % PAGE_SIZE;
        uint64_t aligned_base = base + (align_offset ? (PAGE_SIZE - align_offset) : 0);
        uint64_t aligned_length = length - (aligned_base - base);

        size_t frames = aligned_length / PAGE_SIZE;
        size_t start_frame = aligned_base / PAGE_SIZE;

        for (size_t i = 0; i < frames; i++) {
            if (test_bit(start_frame + i)) { // Only decrement count if it was used
                clear_bit(start_frame + i);
                free_frames_count++;
                used_frames_count--;
            }
        }
    }

    /// @brief Marks a specific region of memory in bitmap as used
    /// @param base Region base physical address
    /// @param length Region length
    void PMM::mark_region_used(uint64_t base, uint64_t length) {
        uint64_t frames = (length + PAGE_SIZE - 1) / PAGE_SIZE; // Round up
        uint64_t start_frame = base / PAGE_SIZE;

        for (uint64_t i = 0; i < frames; i++) {
            if (!test_bit(start_frame + i)) {
                set_bit(start_frame + i);
                free_frames_count--;
                used_frames_count++;
            }
        }
    }

    uint64_t PMM::get_total_memory() { return total_frames * PAGE_SIZE; }
    uint64_t PMM::get_free_memory() { return free_frames_count * PAGE_SIZE; }
    uint64_t PMM::get_used_memory() { return used_frames_count * PAGE_SIZE; }
} // namespace mem
