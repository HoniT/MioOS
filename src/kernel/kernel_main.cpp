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
#include <drivers/vga.hpp>
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
#include <fs/ext/ext2.hpp>
#include <fs/ext/vfs.hpp>
#include <fs/sysdisk.hpp>
#include <tests/unit_tests.hpp>

data::string kernel_version;
extern "C" void kernel_main(const uint32_t magic, void* mbi) {
    // Managing GRUB multiboot error
    if(magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        kernel_panic("Invalid GRUB magic number!");
        return;
    }
    
    vga::init_framebuffer(Multiboot2::get_framebuffer(mbi));

    // Descriptor tables
    gdt::init(); // Global Descriptor Table (GDT)
    idt::init(); // Interrupts Descriptor Table (IDT)
    
    cpu::get_processor_info();
    
    // Initializing memory managers
    heap::init();
    pmm::init(mbi);
    vmm::init();
    kernel_version = "MioOS kernel 0.4 (Alpha)";
    
    // Testing heap
    unittsts::test_heap();
    // Testing PMM
    unittsts::test_pmm();
    // Testing VMM
    unittsts::test_vmm();
    
    // Drivers
    pit::init(); // Programmable Interval Timer
    kbrd::init(); // Keyboard drivers
    
    // Initializing File System, storage drivers...
    device_init();
    ata::init();
    // Finds system disk (the one MioOS is on) and sets up the VFS accordingly
    sysdisk::get_sysdisk(mbi);
    
    // Kernel CLI and other 
    cmd::init();
}
