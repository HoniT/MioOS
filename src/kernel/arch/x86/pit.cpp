// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// pit.cpp
// In charge of setting up the Programmable Interval Timer
// ========================================

#include <pit.hpp>
#include <interrupts/idt.hpp>
#include <drivers/vga_print.hpp>
#include <io.hpp>
#include <lib/math.hpp>

volatile uint64_t ticks;
const uint32_t frequency = 100;

// PIT is IRQ0
void onIrq0(InterruptRegisters* regs) {
    ticks++;
    io::outPortB(0x20, 0x20);  // Send EOI to PIC (if necessary)
}

void pit::init(void) {
    ticks = 0; // Zeroing ticks
    // Installing handler
    idt::irq_install_handler(0, &onIrq0);

    // 119318.16666 Mhz
    uint32_t divisor = 1193182 / frequency;

    io::outPortB(0x43,0x36);
    io::outPortB(0x40,(uint8_t)(divisor & 0xFF));
    io::outPortB(0x40,(uint8_t)((divisor >> 8) & 0xFF));

    vga::printf("PIT initialized!\n");
}

void pit::delay(const uint64_t ms) {
    // A simple delay that waits for `ms` milliseconds (approximated)
    uint32_t start = ticks;
    uint64_t targetTicks = start + ms;  // Target ticks to achieve the delay

    // Wait until the target number of ticks has passed
    while (ticks < targetTicks) {
        // You could add a small NOP here if you want to reduce CPU usage slightly
        __asm__("nop");
    }
}

#pragma region Terminal Functions

void pit::getuptime(void) {
    uint64_t total_seconds = udiv64(ticks, uint64_t(frequency));

    uint64_t hours = udiv64(total_seconds, 3600);
    uint64_t minutes = udiv64(umod64(total_seconds, 3600), 60);
    uint64_t seconds = umod64(total_seconds, 60);

    vga::printf("Hours: %lu\n", hours);
    vga::printf("Minutes: %lu\n", minutes);
    vga::printf("Seconds: %lu\n", seconds);
}

#pragma endregion
