// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// kernel_main.cpp
// In charge of starting and linking the kernel together
// ========================================

#include <kernel_main.hpp>
#include <multiboot.hpp>
#ifndef GFX_HPP
#include <graphics/vga_print.hpp>
#include <drivers/vga.hpp>
#endif // GFX_HPP
#include <drivers/keyboard.hpp>
#include <x86/interrupts/idt.hpp>
#include <x86/interrupts/kernel_panic.hpp>
#include <device.hpp>
#include <x86/gdt.hpp>
#include <x86/cpuid.hpp>
#include <drivers/pit.hpp>
#include <apps/kterminal.hpp>
#include <mm/pmm.hpp>
#include <mm/heap.hpp>
#ifdef PMM_HPP
#include <mm/vmm.hpp>
#endif // PMM_HPP
#include <drivers/ata.hpp>
#include <fs/ext/ext2.hpp>
#include <fs/ext/vfs.hpp>
#include <fs/sysdisk.hpp>
#include <sched/process.hpp>
#include <sched/scheduler.hpp>
#include <drivers/pci.hpp>
#include <tests/unit_tests.hpp>

const char* kernel_version = "MioOS kernel 1.1 (Alpha)";
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
    unittsts::test_heap();
    pmm::init(mbi);
    unittsts::test_pmm();
    vmm::init();
    unittsts::test_vmm();
    
    // Drivers
    pci::pci_brute_force_scan();
    pit::init(); // Programmable Interval Timer
    kbrd::init(); // Keyboard drivers
    
    // Initializing File System, storage drivers...
    device_init();
    ata::init();
    // Finds system disk (the one MioOS is on) and sets up the VFS accordingly
    sysdisk::get_sysdisk(mbi);

    // Scheduler/multitasking
    sched::init();
    
    // Kernel CLI and other
    Process::create(cmd::init, 10, "Kernel Command Line")->start();
    
    for(;;) asm volatile("hlt");
}
