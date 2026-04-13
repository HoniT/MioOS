// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <arch/x86_64/mm/pmm.hpp>
#include <arch/x86_64/multiboot.hpp>
#include <hal/cpu.hpp>
#include <mm/pmm.hpp>

extern "C" uint8_t kernel_start_phys[];
extern "C" uint8_t kernel_end_phys[];

void arch::mem::init_pmm(void* mbi) {
    multiboot_tag_mmap* mmap_tag = Multiboot2::get_mmap(mbi);
    if (!mmap_tag) {
        hal::cpu::halt();
        return;
    }

    // Find Top of Memory to scale bitmap accordingly
    uint32_t num_entries = (mmap_tag->size - sizeof(multiboot_tag_mmap)) / mmap_tag->entry_size;
    uint64_t top_physical_memory = 0;
    for (uint32_t i = 0; i < num_entries; i++) {
        multiboot_mmap_entry* entry = &mmap_tag->entries[i];
        if (entry->addr + entry->len > top_physical_memory) {
            top_physical_memory = entry->addr + entry->len;
        }
    }

    // Initialize the Universal PMM Backend
    uint64_t bitmap_phys_addr = (uint64_t)kernel_end_phys;
    void* bitmap_virt = reinterpret_cast<void*>(bitmap_phys_addr + ::mem::HIGHER_HALF_OFFSET);
    
    ::mem::PMM::init(bitmap_virt, top_physical_memory);

    // Mark available regions as free based on GRUB's x86_64 memory map
    for (uint32_t i = 0; i < num_entries; i++) {
        multiboot_mmap_entry* entry = &mmap_tag->entries[i];
        if (entry->type == 1) { // MULTIBOOT_MEMORY_AVAILABLE
            ::mem::PMM::mark_region_free(entry->addr, entry->len);
        }
    }

    // Explicitly protect crucial physical memory regions
    uint64_t kernel_size = (uint64_t)kernel_end_phys - (uint64_t)kernel_start_phys;
    ::mem::PMM::mark_region_used((uint64_t)kernel_start_phys, kernel_size);

    uint64_t total_frames = top_physical_memory / ::mem::PAGE_SIZE;
    uint64_t bitmap_size_bytes = (total_frames / 8) + 1;
    ::mem::PMM::mark_region_used(bitmap_phys_addr, bitmap_size_bytes);

    uint64_t mb2_phys = (uint64_t)mbi - ::mem::HIGHER_HALF_OFFSET;
    uint32_t mb2_size = *reinterpret_cast<uint32_t*>(mbi);
    ::mem::PMM::mark_region_used(mb2_phys, mb2_size);
}
