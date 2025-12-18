// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// pit.cpp
// In charge of setting up the Programmable Interval Timer
// ========================================

#include <drivers/pit.hpp>
#include <x86/interrupts/idt.hpp>
#include <x86/interrupts/kernel_panic.hpp>
#include <graphics/vga_print.hpp>
#include <x86/io.hpp>
#include <x86/interrupts/pic.hpp>
#include <sched/scheduler.hpp>
#include <lib/math.hpp>

volatile uint64_t ticks;  
const uint32_t frequency = 1000; // Hz

// PIT is IRQ0
void onIrq0(InterruptRegisters* regs) {
    ticks++;

    // Acknowledge the interrupt BEFORE scheduling.
    // If we switch tasks, the new task needs to be able to receive interrupts.
    // If we don't EOI, the PIC remains blocked on IRQ0.
    pic::send_eoi(PIT_IRQ);

    // Decrement current task's time slice
    if (curr_process && curr_process->get_state() == PROCESS_RUNNING) {
        curr_process->decrement_time_slice();
        
        // If time slice expired, trigger rescheduling
        if (curr_process->get_time_slice() == 0) {
            sched::schedule();
        }
    }
}

void pit::init(void) {
    ticks = 0; // Zeroing ticks
    // Installing handler
    idt::irq_install_handler(PIT_IRQ, &onIrq0);

    // 1193.182 Mhz
    uint32_t divisor = 1193182 / frequency;

    io::outPortB(0x43, 0x36);
    io::outPortB(0x40, (uint8_t)(divisor & 0xFF));
    io::outPortB(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    if(!idt::check_irq(PIT_IRQ, &onIrq0)) {
        kprintf(LOG_ERROR, "Failed to initialize Programmable Interval Timer! (IRQ 0 not installed)\n");
        kernel_panic("Fatal component failed to initialize!");
    }
    kprintf(LOG_INFO, "Implemented Programmable Interval Timer\n");
}

void pit::delay(const uint64_t ms) {
    // A simple delay that waits for `ms` milliseconds (approximated)
    uint32_t start = ticks;
    uint64_t targetTicks = start + udiv64((ms * frequency), 1000);  // Target ticks to achieve the delay

    // Wait until the target number of ticks has passed
    while (ticks < targetTicks) {
        __asm__("nop");
    }
}

#pragma region Terminal Functions

void pit::getuptime(data::list<data::string> params) {
    uint64_t total_seconds = udiv64(ticks, uint64_t(frequency));

    uint64_t hours = udiv64(total_seconds, 3600);
    uint64_t minutes = udiv64(umod64(total_seconds, 3600), 60);
    uint64_t seconds = umod64(total_seconds, 60);

    kprintf("Hours: %llu:%llu:%llu\n", hours, minutes, seconds);
}

#pragma endregion
