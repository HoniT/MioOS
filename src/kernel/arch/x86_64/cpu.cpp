// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <cpu.hpp>

void hal::cpu::halt(void) {
    for(;;) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}

void hal::cpu::enable_interrupts(void) {
    asm volatile("sti");
}

void hal::cpu::disable_interrupts(void) {
    asm volatile("cli");
}
