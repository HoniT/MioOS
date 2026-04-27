// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <arch/x86_64/mm/pmm_setup.hpp>
#include <arch/x86_64/multiboot.hpp>
#include <cpu.hpp>
#include <mm/pmm.hpp>

using mem::PMM;

extern "C" uint8_t kernel_start_phys[];
extern "C" uint8_t kernel_end_phys[];

void arch::mem::init_pmm(void* mbi) {
    multiboot_tag_mmap* mmap_tag = Multiboot2::get_mmap(mbi);
    if (!mmap_tag) {
        hal::cpu::halt();
        return;
    }

    // 1. Calculate top of physical memory
    uint64_t top_physical_memory = 0;
    for (uint8_t* ptr = (uint8_t*)mmap_tag->entries; 
        ptr < (uint8_t*)mmap_tag + mmap_tag->size; 
        ptr += mmap_tag->entry_size) {
        multiboot_mmap_entry* entry = (multiboot_mmap_entry*)ptr;
        if (entry->addr + entry->len > top_physical_memory) {
            top_physical_memory = entry->addr + entry->len;
        }
    }

    // 2. Safely place the bitmap AFTER both the kernel and the MBI structure
    uint64_t kernel_end = (uint64_t)kernel_end_phys;
    uint64_t mb2_phys = (uint64_t)mbi - ::mem::HIGHER_HALF_OFFSET;
    uint32_t mb2_size = *reinterpret_cast<uint32_t*>(mbi);
    uint64_t mb2_end = mb2_phys + mb2_size;

    // Pick the highest address to avoid overlapping GRUB data
    uint64_t bitmap_phys_addr = (kernel_end > mb2_end) ? kernel_end : mb2_end;
    
    // Page-align the start of the bitmap for safety/cleanliness
    bitmap_phys_addr = (bitmap_phys_addr + ::mem::FRAME_SIZE - 1) & ~(::mem::FRAME_SIZE - 1);

    void* bitmap_virt = reinterpret_cast<void*>(bitmap_phys_addr + ::mem::HIGHER_HALF_OFFSET);
    
    // 3. Initialize PMM (Sets everything to ~0ULL)
    PMM::init(bitmap_virt, top_physical_memory);
    
    // 4. Mark available regions free
    for (uint8_t* ptr = (uint8_t*)mmap_tag->entries; 
        ptr < (uint8_t*)mmap_tag + mmap_tag->size; 
        ptr += mmap_tag->entry_size) {
        multiboot_mmap_entry* entry = (multiboot_mmap_entry*)ptr;
        if (entry->type == 1) { // MULTIBOOT_MEMORY_AVAILABLE
            PMM::mark_region_free(entry->addr, entry->len);
        }
    }

    // 5. Explicitly protect crucial physical memory regions
    PMM::mark_region_used(0x0, 0x100000); // Protect low memory (VGA, IVT, BIOS data)
    
    uint64_t kernel_size = (uint64_t)kernel_end_phys - (uint64_t)kernel_start_phys;
    PMM::mark_region_used((uint64_t)kernel_start_phys, kernel_size);
    
    uint64_t total_frames = top_physical_memory / ::mem::FRAME_SIZE;
    uint64_t bitmap_size_bytes = ((total_frames + 63) / 64) * 8; 
    PMM::mark_region_used(bitmap_phys_addr, bitmap_size_bytes);
    
    PMM::mark_region_used(mb2_phys, mb2_size);
}