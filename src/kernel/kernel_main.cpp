// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <kernel_main.hpp>
#include <hal/cpu.hpp>

const char* kernel_version = "MioOS kernel 2.0";

void kernel_main() {
    // Halting forever
    hal::cpu::halt();
}
