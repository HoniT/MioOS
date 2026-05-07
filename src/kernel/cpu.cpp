// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <cpu.hpp>

[[noreturn]] void cpu::halt() {
    for(;;) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}

void cpu::enable_interrupts() { asm volatile("sti"); }

void cpu::disable_interrupts() { asm volatile("cli"); }
