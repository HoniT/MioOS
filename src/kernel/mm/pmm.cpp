// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Physical memory manager
// Physical RAM bitmap and allocation/deallocation API
// ========================================

#include <mm/pmm.hpp>
#include <multiboot.hpp>
#include <kernel_panic.hpp>
#include <graphics/kprint.hpp>

// Linker symbols
extern "C" uint8_t kernel_start_phys[];
extern "C" uint8_t kernel_end_phys[];

namespace mem
{
    PhysAddr* PMM::bitmap = nullptr;
    size_t PMM::bitmap_size_bytes = 0;
    size_t PMM::total_frames = 0;
    size_t PMM::free_frames_count = 0;
    size_t PMM::used_frames_count = 0;
    uint64_t PMM::last_scanned_index = 0; // Optimization for fast allocation

    // Bitmap helpers

    void PMM::set_bit(uint64_t bit) {
        bitmap[bit / 64] |= (1ULL << (bit % 64));
    }

    void PMM::clear_bit(uint64_t bit) {
        bitmap[bit / 64] &= ~(1ULL << (bit % 64));
    }

    bool PMM::test_bit(uint64_t bit) {
        return bitmap[bit / 64] & (1ULL << (bit % 64));
    }

    void PMM::init(void* mbi) {
        // Getting the MMAP from GRUB
        multiboot_tag_mmap* mmap_tag = Multiboot2::get_mmap(mbi);
        if (!mmap_tag) {
            kernel_panic("PMM: No memory map passed by GRUB\n");
            return;
        }

        // Calculate top of physical memory
        uint64_t top_physical_memory = 0;
        for (uint8_t* ptr = (uint8_t*)mmap_tag->entries; 
                ptr < (uint8_t*)mmap_tag + mmap_tag->size; 
                ptr += mmap_tag->entry_size) {
            multiboot_mmap_entry* entry = (multiboot_mmap_entry*)ptr;
            if (entry->addr + entry->len > top_physical_memory) {
                top_physical_memory = entry->addr + entry->len;
            }
        }

        // Safely place the bitmap AFTER both the kernel and the MBI structure
        uint64_t kernel_end = (uint64_t)kernel_end_phys;
        uint64_t mb2_phys = (uint64_t)mbi - HIGHER_HALF_OFFSET;
        uint32_t mb2_size = *reinterpret_cast<uint32_t*>(mbi);
        uint64_t mb2_end = mb2_phys + mb2_size;

        // Pick the highest address to avoid overlapping GRUB data
        uint64_t bitmap_phys_addr = (kernel_end > mb2_end) ? kernel_end : mb2_end;
        bitmap_phys_addr = (bitmap_phys_addr + FRAME_SIZE - 1) & ~(FRAME_SIZE - 1); // Page aligning
        void* bitmap_virt = reinterpret_cast<void*>(bitmap_phys_addr + HIGHER_HALF_OFFSET);

        total_frames = top_physical_memory / FRAME_SIZE;
        uint64_t bitmap_entries = (total_frames + 63) / 64; 
        bitmap_size_bytes = bitmap_entries * 8;
        bitmap = reinterpret_cast<uint64_t*>(bitmap_virt);
        
        // Set everything as allocated for safety
        for (size_t i = 0; i < bitmap_entries; i++) {
            bitmap[i] = ~0ULL; 
        }
        used_frames_count = total_frames;
        free_frames_count = 0;

        // Now marking only the available regions free
        for (uint8_t* ptr = (uint8_t*)mmap_tag->entries; 
            ptr < (uint8_t*)mmap_tag + mmap_tag->size; 
            ptr += mmap_tag->entry_size) {
            multiboot_mmap_entry* entry = (multiboot_mmap_entry*)ptr;
            if (entry->type == 1) { // MULTIBOOT_MEMORY_AVAILABLE
                PMM::mark_region_free(entry->addr, entry->len);
            }
        }

        // Explicitly protecting crucial memory regions for safety
        PMM::mark_region_used(0x0, 0x100000); // Protect low memory (VGA, IVT, BIOS data)
    
        uint64_t kernel_size = (uint64_t)kernel_end_phys - (uint64_t)kernel_start_phys;
        PMM::mark_region_used((uint64_t)kernel_start_phys, kernel_size); // Kernel
        
        uint64_t total_frames = top_physical_memory / ::mem::FRAME_SIZE;
        uint64_t bitmap_size_bytes = ((total_frames + 63) / 64) * 8; 
        PMM::mark_region_used(bitmap_phys_addr, bitmap_size_bytes); // PMM Bitmap
        
        PMM::mark_region_used(mb2_phys, mb2_size); // Mb2 info

        kprintf("PMM: Physical memory manager initialized\n");
    }

    void* PMM::alloc_frame() {
        lock();

        for (uint64_t i = last_scanned_index; i < (total_frames + 63) / 64; i++) {
            if (bitmap[i] != ~0ULL) {
                int bit = __builtin_ffsll(~bitmap[i]) - 1;
                uint64_t frame_index = (i * 64) + bit;
                
                if (frame_index >= total_frames) break; 
                
                set_bit(frame_index);
                last_scanned_index = i;
                free_frames_count--;
                used_frames_count++;
                
                unlock();
                return reinterpret_cast<void*>(frame_index * FRAME_SIZE);
            }
        }

        if (last_scanned_index > 0) {
            last_scanned_index = 0;
            unlock();
            return alloc_frame(); 
        }

        unlock();
        return nullptr;
    }

    void* PMM::alloc_frames(size_t count) {
        if(count <= 0) return nullptr;

        lock();

        uint64_t start_frame = 0;
        uint64_t consecutive_free = 0;
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
                    return reinterpret_cast<void*>(start_frame * FRAME_SIZE);
                }
            } else {
                consecutive_free = 0; 
            }
        }

        unlock();
        return nullptr;
    }

    void PMM::free_frame(void* phys_addr) {
        if(phys_addr == nullptr) return;

        lock();

        uint64_t frame_index = reinterpret_cast<uint64_t>(phys_addr) / FRAME_SIZE;
        if (test_bit(frame_index)) {
            clear_bit(frame_index);

            // For statistics
            free_frames_count++;
            used_frames_count--;
            
            if (frame_index / 64 < last_scanned_index) {
                last_scanned_index = frame_index / 64;
            }
        }

        unlock();
    }

    void PMM::free_frames(void* phys_addr, size_t count) {
        if(count <= 0) return;

        lock();

        uint64_t start_frame = reinterpret_cast<uint64_t>(phys_addr) / FRAME_SIZE;
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

    void PMM::mark_region_free(PhysAddr base, size_t length) {
        if(length <= 0) return;

        lock();

        uint64_t align_offset = base % FRAME_SIZE;
        uint64_t aligned_base = base + (align_offset ? (FRAME_SIZE - align_offset) : 0);

        if (base + length <= aligned_base) return;

        uint64_t aligned_length = (base + length) - aligned_base;
        size_t frames = aligned_length / FRAME_SIZE;
        size_t start_frame = aligned_base / FRAME_SIZE;

        for (size_t i = 0; i < frames; i++) {
            if (test_bit(start_frame + i)) { 
                clear_bit(start_frame + i);
                free_frames_count++;
                used_frames_count--;
            }
        }

        unlock();
    }

    void PMM::mark_region_used(PhysAddr base, size_t length) {
        if(length <= 0) return;

        lock();

        uint64_t start_frame = base / FRAME_SIZE;
        uint64_t end_frame = (base + length + FRAME_SIZE - 1) / FRAME_SIZE; 
        uint64_t frames = end_frame - start_frame;

        for (uint64_t i = 0; i < frames; i++) {
            if (!test_bit(start_frame + i)) {
                set_bit(start_frame + i);
                free_frames_count--;
                used_frames_count++;
            }
        }

        unlock();
    }

    void PMM::lock() { /* TODO */ }
    void PMM::unlock() { /* TODO */ }

    size_t PMM::get_total_memory() { return total_frames * FRAME_SIZE; }
    size_t PMM::get_free_memory() { return free_frames_count * FRAME_SIZE; }
    size_t PMM::get_used_memory() { return used_frames_count * FRAME_SIZE; }
} // namespace mem
