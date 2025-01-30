// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// kernel_panic.cpp
// In charge of kernel panics
// ========================================

#include <interrupts/kernel_panic.hpp>
#include <drivers/vga_print.hpp>

// Kernel panic manager
void kernel_panic(const char* error) {
    asm volatile ("cli"); // Clearing interrupts

    vga::printf("%s\nException! system Halted\n", error);

    // Will save dump in the future, instead of infinite loop
    for(;;) {}

    return;
}

void kernel_panic_isr(const char* error) {
    // Page faults are handled by a page fault handler in vmm.cpp
    asm volatile ("cli"); // Clearing interrupts

    // Zeroing out VGA columb and row so it writes the error at the start of the screen
    col = row = 0;
        
    vga::print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_CYAN);
    vga::print_clear();

    vga::printf("%s\nException! system Halted\n", error);

    // Will save dump in the future, instead of infinite loop
    for(;;) {}

    return;
}
