// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <arch/x86_64/cpu.hpp>

void cpu::halt(void) {
    for(;;) asm volatile("hlt");
}
