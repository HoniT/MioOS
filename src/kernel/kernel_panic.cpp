// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <kernel_panic.hpp>
#include <cpu.hpp>

void kernel_panic() {
    // Just a wrapper that only halts for now 
    cpu::halt();
}
