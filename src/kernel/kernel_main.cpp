// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Kernel entry point and initialization
// ========================================

#include <kernel_main.hpp>
#include <multiboot.hpp>
#include <cpu.hpp>
#include <kernel_panic.hpp>
#include <mm/pmm.hpp>
#include <graphics/kprint.hpp>

extern "C" void kernel_main(void* mbi, uint32_t magic) {
    if(magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        kernel_panic("Invalid Multiboot2 magic detected\n");
        return;
    }

    mem::PMM::init(mbi);

    kprintf("Kernel initialization finished\n");
    cpu::halt();
}