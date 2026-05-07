// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <kernel_main.hpp>
#include <multiboot.hpp>
#include <cpu.hpp>
#include <kernel_panic.hpp>
#include <mm/pmm.hpp>

extern "C" void kernel_main(void* mbi, uint32_t magic) {
    if(magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        kernel_panic();
        return;
    }

    mem::PMM::init(mbi);

    uint16_t* vga_buffer = (uint16_t*)0xFFFFFFFF800B8000;
    const char* str = "Successfully booted into 64-bit Higher-Half Long Mode!";
    for (int i = 0; str[i] != '\0'; ++i)
        vga_buffer[i] = (uint16_t)str[i] | ((uint16_t)15 << 8); 

    cpu::halt();
}