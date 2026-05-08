// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Handles kernel panics
// ========================================

#include <kernel_panic.hpp>
#include <cpu.hpp>
#include <graphics/kprint.hpp>

void kernel_panic(const char* msg, ...) {
    // Printing message
    kprintf("OS triggered KERNEL PANIC. Halting system.\n");
    va_list args;
    va_start(args, msg);
    kvprintf(msg, args);
    va_end(args);

    // Just a wrapper that only halts for now 
    cpu::halt();
}
