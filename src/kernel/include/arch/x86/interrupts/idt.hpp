// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef IDT_MAIN_HPP
#define IDT_MAIN_HPP

#include <stdint.h>

#define IDT_SIZE 256
#define IRQ_QUANTITY 16

#define PAGE_FAULT_INDEX 14

// This ALWAYS NEEDS TO BE 8 BYTES IN TOTAL, because we have a 32 bit OS
struct idt_entry {
    uint16_t offset_1; // Offset bits 0-15
    uint16_t selector; // A code segment selector in the GDT
    uint8_t zero; // This is always zero. Unused
    uint8_t gate_attributes; // Gate type, DPL and P fields
    uint16_t offset_2; // Offset bits 16-31
} __attribute__((packed));

// Pointer struct
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Registers related to an interrupt (ISR)
struct InterruptRegistersISR {
    uint32_t ds;               // Pushed manually in isr_common_stub
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pusha
    uint32_t interr_no;        // Interrupt number
    uint32_t err_code;         // Error code (only for some exceptions)
    uint32_t eip, cs, eflags, useresp, ss; // Pushed by CPU
} __attribute__((packed));

// Registers related to an interrupt (IRQ)
struct InterruptRegisters {
    uint32_t cr2;
    uint32_t ds;               // Pushed manually in isr_common_stub
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pusha
    uint32_t interr_no;        // Interrupt number
    uint32_t err_code;         // Error code (only for some exceptions)
    uint32_t eip, cs, eflags, useresp, ss; // Pushed by CPU
} __attribute__((packed));

namespace idt {
// Functions
void init(void); // Initializes IDT
// Sets an IDT gate
void set_idt_gate(const uint8_t num, const uint32_t base, const uint16_t selector, const uint8_t flags);
// Installing and uninstalling IRQ handler
extern "C" void irq_install_handler(const int irq_num, void (*handler)(struct InterruptRegisters* regs));
extern "C" void irq_uninstall_handler(const int irq_num);

extern const char* exception_messages[];

} // Namespace idt

// Handlers
extern "C" void isr_handler(struct InterruptRegistersISR* regs);
extern "C" void irq_handler(struct InterruptRegisters* regs);
// Flushes the IDT
extern "C" void idt_flush(uint32_t);

// ISRs and IRQs
extern "C" {

// ====================
// Interrupt Service Routines
// ====================

    void isr0();
    void isr1();
    void isr2();
    void isr3();
    void isr4();
    void isr5();
    void isr6();
    void isr7();
    void isr8();
    void isr9();
    void isr10();
    void isr11();
    void isr12();
    void isr13();
    void isr14();
    void isr15();
    void isr16();
    void isr17();
    void isr18();
    void isr19();
    void isr20();
    void isr21();
    void isr22();
    void isr23();
    void isr24();
    void isr25();
    void isr26();
    void isr27();
    void isr28();
    void isr29();
    void isr30();
    void isr31();
    // Syscalls
    void isr128();
    void isr177();

// ====================
// Interrupt Service Routines
// ====================

    void irq0();
    void irq1();
    void irq2();
    void irq3();
    void irq4();
    void irq5();
    void irq6();
    void irq7();
    void irq8();
    void irq9();
    void irq10();
    void irq11();
    void irq12();
    void irq13();
    void irq14();
    void irq15();
}

#endif // IDT_MAIN_HPP
