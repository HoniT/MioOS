// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <kernel_main.hpp>
#include <multiboot.hpp>
#include <hal/cpu.hpp>
#include <mm/pmm.hpp>

extern "C" uint64_t p4_table[];

const char* kernel_version = "MioOS kernel 2.0";
extern "C" void kernel_main(void* mbi, uint32_t magic) {
    // Managing GRUB multiboot error
    if(magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        hal::cpu::halt();
        return;
    }
    
    // Destroy the lower-half identity map
    p4_table[0] = 0;

    // Flush the TLB the safe way
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    __asm__ volatile("mov %0, %%cr3" :: "r" (cr3));

    // Memory manager init
    mem::PMM::init(mbi);

    // 3. Print to the screen to prove we survived!
    // Physical VGA buffer is at 0xB8000
    // Higher-half virtual address is 0xFFFFFFFF80000000 + 0xB8000
    uint16_t* vga_buffer = (uint16_t*)0xFFFFFFFF800B8000;
    
    const char* str = "Successfully booted into 64-bit Higher-Half Long Mode!";
    
    for (int i = 0; str[i] != '\0'; ++i) {
        // Character | Color (White text [15] on Black background [0])
        vga_buffer[i] = (uint16_t)str[i] | ((uint16_t)15 << 8); 
    }

    // Halt the CPU forever
    hal::cpu::halt();
}