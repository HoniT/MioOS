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
#include <gdt.hpp>
#include <cpuid.hpp>
#include <pit.hpp>
#include <kterminal.hpp>

extern "C" void kernel_main(void) {
    vga::init(); // Setting main VGA text/title

    gdt::init(); // Global Descriptor Table (GDT)
    idt::init(); // Interrupts Descriptor Table (IDT)

    cpu::init(); // CPUID

    // Drivers
    pit::init(); // Programmable Interval Timer
    keyboard::init(); // Keyboard drivers
    
    cmd::init();
}
