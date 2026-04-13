// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <arch/x86_64/entry.hpp>
#include <arch/x86_64/multiboot.hpp>
#include <hal/cpu.hpp>
#include <arch/x86_64/mm/pmm.hpp>
#include <kernel_main.hpp>

/// @brief Entry point for x86_64 arch
/// @param mbi Multiboot2 info structure
/// @param magic GRUB magic
extern "C" void entry_x86_64(void* mbi, uint32_t magic) {
    // Managing GRUB multiboot error
    if(magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        hal::cpu::halt();
        return;
    }
    
    // Memory manager init
    arch::mem::init_pmm(mbi);

    // 3. Print to the screen to prove we survived!
    // Physical VGA buffer is at 0xB8000
    // Higher-half virtual address is 0xFFFFFFFF80000000 + 0xB8000
    uint16_t* vga_buffer = (uint16_t*)0xFFFFFFFF800B8000;
    
    const char* str = "Successfully booted into 64-bit Higher-Half Long Mode!";
    
    for (int i = 0; str[i] != '\0'; ++i) {
        // Character | Color (White text [15] on Black background [0])
        vga_buffer[i] = (uint16_t)str[i] | ((uint16_t)15 << 8); 
    }

    // Calling arch independent main function
    kernel_main();
}