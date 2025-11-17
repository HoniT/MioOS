// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// pic.cpp
// Programmable Interrupt Controller functions
// ========================================

#include <x86/interrupts/pic.hpp>
#include <x86/io.hpp>

// Sends End of Interrupt signal for a given interrupt
void pic::send_eoi(const uint8_t irq) {
    if (irq >= 8)
        io::outPortB(PIC_SLAVE_COMMAND, EOI);   // Notify slave PIC if IRQ is 8–15
    io::outPortB(PIC_MASTER_COMMAND, EOI);      // Always notify master PIC
}

void pic::unmask_irq(const uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC_MASTER_DATA : PIC_SLAVE_DATA;
    uint8_t value = io::inPortB(port) & ~(1 << (irq % 8));
    io::outPortB(port, value);
}