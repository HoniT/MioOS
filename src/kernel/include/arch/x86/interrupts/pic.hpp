// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef PIC_HPP
#define PIC_HPP

#include <stdint.h>

// Macros for common PIC ports/commands

// Ports
#define PIC_MASTER_COMMAND 0x20
#define PIC_MASTER_DATA    0x21
#define PIC_SLAVE_COMMAND 0xA0
#define PIC_SLAVE_DATA    0xA1

// Initialization command words
#define ICW1_INIT    0x11
#define ICW4_8086    0x01

#define EOI 0x20

namespace pic {
    // Sends End of Interrupt signal for a given IRQ
    void send_eoi(const uint8_t irq);
    // Unmasks IRQ
    void unmask_irq(const uint8_t irq);
}

#endif // PIC_HPP
