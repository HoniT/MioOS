// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// kernel_main.cpp
// In charge of starting and linking the kernel together
// ========================================

#include <kernel_main.hpp>
#ifndef GFX_HPP
#include <drivers/vga_print.hpp>
#endif // GFX_HPP
#include <drivers/keyboard.hpp>
#include <interrupts/idt.hpp>
#include <interrupts/kernel_panic.hpp>
#include <device.hpp>
#include <gdt.hpp>
#include <cpuid.hpp>
#include <pit.hpp>
#include <kterminal.hpp>
#include <mm/pmm.hpp>
#include <mm/heap.hpp>
#ifdef PMM_HPP
#include <mm/vmm.hpp>
#endif // PMM_HPP
#include <drivers/ata.hpp>
#include <fs/ext2.hpp>
#include <fs/vfs.hpp>
#include <lib/string_util.hpp>
#include <tests/unit_tests.hpp>

extern "C" void kernel_main(uint32_t magic, multiboot_info* mbi) {
    // Managing GRUB multiboot error
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        vga::error("Ivalid multiboot magic number: %x\n", magic);
        kernel_panic("Invalid GRUB magic number!");
        return;
    }

    vga::init(); // Setting main VGA text/title
    
    // Descriptor tables
    gdt::init(); // Global Descriptor Table (GDT)
    idt::init(); // Interrupts Descriptor Table (IDT)
    
    // Initializing memory managers
    heap::init();
    pmm::init(mbi);
    vmm::init();
    
    cpu::get_processor_info(); // Printing CPU vendor and model

    // Testing heap
    unittsts::test_heap();
    // Testing PMM
    unittsts::test_pmm();
    // Testing VMM
    unittsts::test_vmm();

    // Drivers
    pit::init(); // Programmable Interval Timer
    keyboard::init(); // Keyboard drivers

    // Initializing File System, storage drivers...
    device_init();
    ata::init();
    vfs::init();
    // Initializing Ext2 on ATA devices
    for(int i = 0; i < sizeof(ata_devices); i++)
        if(strcmp(ata_devices[i].serial, "") != 0)
            ext2::init_ext2_device(&ata_devices[i]);
    
    // Kernel CLI and other 
    cmd::init();
}
