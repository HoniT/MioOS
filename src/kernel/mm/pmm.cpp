// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <arch/x86_64/mm/pmm_setup.hpp>
#include <arch/x86_64/multiboot.hpp>
#include <cpu.hpp>
#include <mm/pmm.hpp>
#include <arch/x86_64/entry.hpp>

using mem::PMM;

namespace mem
{
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

    void PMM::init(void* bitmap_virt_addr, uint64_t top_physical_memory) {
        total_frames = top_physical_memory / FRAME_SIZE;
        uint64_t bitmap_entries = (total_frames + 63) / 64; 
        bitmap_size_bytes = bitmap_entries * 8;
        bitmap = reinterpret_cast<uint64_t*>(bitmap_virt_addr);
        
        for (size_t i = 0; i < bitmap_entries; i++) {
            bitmap[i] = ~0ULL; 
        }
        used_frames_count = total_frames;
        free_frames_count = 0;
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

    void* PMM::alloc_frames(uint64_t count) {
        if (count == 0) return nullptr;
        
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
        lock();
        uint64_t frame_index = reinterpret_cast<uint64_t>(phys_addr) / FRAME_SIZE;
        if (test_bit(frame_index)) {
            clear_bit(frame_index);
            free_frames_count++;
            used_frames_count--;
            
            if (frame_index / 64 < last_scanned_index) {
                last_scanned_index = frame_index / 64;
            }
        }
        unlock();
    }

    void PMM::free_frames(void* phys_addr, uint64_t count) {
        uint64_t start_frame = reinterpret_cast<uint64_t>(phys_addr) / FRAME_SIZE;
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

    void PMM::mark_region_free(uint64_t base, uint64_t length) {
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
    }

    void PMM::mark_region_used(uint64_t base, uint64_t length) {
        if (length == 0) return;

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
    }

    uint64_t PMM::get_total_memory() { return total_frames * FRAME_SIZE; }
    uint64_t PMM::get_free_memory() { return free_frames_count * FRAME_SIZE; }
    uint64_t PMM::get_used_memory() { return used_frames_count * FRAME_SIZE; }
} // namespace mem