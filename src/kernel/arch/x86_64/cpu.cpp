// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <hal/cpu.hpp>

void hal::cpu::halt(void) {
    for(;;) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}
