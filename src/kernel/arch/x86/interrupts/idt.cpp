// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// idt.cpp
// In charge of setting up the Interrupts Descriptor Table
// ========================================

#include <interrupts/idt.hpp>
#include <drivers/vga_print.hpp>
#include <interrupts/kernel_panic.hpp>
#include <pic.hpp>
#include <io.hpp>
#include <lib/mem_util.hpp>

using io::outPortB;

__attribute__((aligned(8))) idt_entry idt_entries[IDT_SIZE]; // 256 IDT entries in total
struct idt_ptr idt_ptr;


// Initializing IDT
void idt::init() {
    idt_ptr.limit = sizeof(idt_entry) * 256 - 1; // Same operation as gdt_descriptor in boot.asm
    idt_ptr.base = (uint32_t)&idt_entries;

    // Initialize everything to 0
    memset(&idt_entries, 0, sizeof(idt_entry) * 256);

    // Sending data to chips using outPortB defined in util.cpp
    // Configuring the Programmable Interrupt Controller (PIC)

    outPortB(PIC_MASTER_COMMAND, ICW1_INIT);
    outPortB(PIC_SLAVE_COMMAND, ICW1_INIT);
    outPortB(PIC_MASTER_DATA, 0x20);
    outPortB(PIC_SLAVE_DATA, 0x28);
    outPortB(PIC_MASTER_DATA, 0x04);
    outPortB(PIC_SLAVE_DATA, 0x02);
    outPortB(PIC_MASTER_DATA, ICW4_8086);
    outPortB(PIC_SLAVE_DATA, ICW4_8086);
    outPortB(PIC_MASTER_DATA, 0x0);
    outPortB(PIC_SLAVE_DATA, 0x0);

    // Setting every gate up
    // These all are 32 bit Interrupt Gates based off of the flag 0x8E

    set_idt_gate(0, uint32_t(isr0), 0x08, 0x8E);
    set_idt_gate(1, uint32_t(isr1), 0x08, 0x8E);
    set_idt_gate(2, uint32_t(isr2), 0x08, 0x8E);
    set_idt_gate(3, uint32_t(isr3), 0x08, 0x8E);
    set_idt_gate(4, uint32_t(isr4), 0x08, 0x8E);
    set_idt_gate(5, uint32_t(isr5), 0x08, 0x8E);
    set_idt_gate(6, uint32_t(isr6), 0x08, 0x8E);
    set_idt_gate(7, uint32_t(isr7), 0x08, 0x8E);
    set_idt_gate(8, uint32_t(isr8), 0x08, 0x8E);
    set_idt_gate(9, uint32_t(isr9), 0x08, 0x8E);
    set_idt_gate(10, uint32_t(isr10), 0x08, 0x8E);
    set_idt_gate(11, uint32_t(isr11), 0x08, 0x8E);
    set_idt_gate(12, uint32_t(isr12), 0x08, 0x8E);
    set_idt_gate(13, uint32_t(isr13), 0x08, 0x8E);
    set_idt_gate(14, uint32_t(isr14), 0x08, 0x8E);
    set_idt_gate(15, uint32_t(isr15), 0x08, 0x8E);
    set_idt_gate(16, uint32_t(isr16), 0x08, 0x8E);
    set_idt_gate(17, uint32_t(isr17), 0x08, 0x8E);
    set_idt_gate(18, uint32_t(isr18), 0x08, 0x8E);
    set_idt_gate(19, uint32_t(isr19), 0x08, 0x8E);
    set_idt_gate(20, uint32_t(isr20), 0x08, 0x8E);
    set_idt_gate(21, uint32_t(isr21), 0x08, 0x8E);
    set_idt_gate(22, uint32_t(isr22), 0x08, 0x8E);
    set_idt_gate(23, uint32_t(isr23), 0x08, 0x8E);
    set_idt_gate(24, uint32_t(isr24), 0x08, 0x8E);
    set_idt_gate(25, uint32_t(isr25), 0x08, 0x8E);
    set_idt_gate(26, uint32_t(isr26), 0x08, 0x8E);
    set_idt_gate(27, uint32_t(isr27), 0x08, 0x8E);
    set_idt_gate(28, uint32_t(isr28), 0x08, 0x8E);
    set_idt_gate(29, uint32_t(isr29), 0x08, 0x8E);
    set_idt_gate(30, uint32_t(isr30), 0x08, 0x8E);
    set_idt_gate(31, uint32_t(isr31), 0x08, 0x8E);

    set_idt_gate(128, uint32_t(isr128), 0x08, 0x8E); // System calls
    set_idt_gate(177, uint32_t(isr177), 0x08, 0x8E); // System calls


    set_idt_gate(32, uint32_t(irq0), 0x08, 0x8E);
    set_idt_gate(33, uint32_t(irq1), 0x08, 0x8E);
    set_idt_gate(34, uint32_t(irq2), 0x08, 0x8E);
    set_idt_gate(35, uint32_t(irq3), 0x08, 0x8E);
    set_idt_gate(36, uint32_t(irq4), 0x08, 0x8E);
    set_idt_gate(37, uint32_t(irq5), 0x08, 0x8E);
    set_idt_gate(38, uint32_t(irq6), 0x08, 0x8E);
    set_idt_gate(39, uint32_t(irq7), 0x08, 0x8E);
    set_idt_gate(40, uint32_t(irq8), 0x08, 0x8E);
    set_idt_gate(41, uint32_t(irq9), 0x08, 0x8E);
    set_idt_gate(42, uint32_t(irq10), 0x08, 0x8E);
    set_idt_gate(43, uint32_t(irq11), 0x08, 0x8E);
    set_idt_gate(44, uint32_t(irq12), 0x08, 0x8E);
    set_idt_gate(45, uint32_t(irq13), 0x08, 0x8E);
    set_idt_gate(46, uint32_t(irq14), 0x08, 0x8E);
    set_idt_gate(47, uint32_t(irq15), 0x08, 0x8E);

    // Flushing IDT
    idt_flush((uint32_t)&idt_ptr);
    vga::printf("Implemented IDT at %x!\n", idt_ptr.base);
}

// Sets an IDT gate
void idt::set_idt_gate(const uint8_t num, const uint32_t base, const uint16_t selector, const uint8_t flags) {
    // Offsets
    idt_entries[num].offset_1 = base & 0xFFFF;
    idt_entries[num].offset_2 = (base >> 16) & 0xFFFF;

    idt_entries[num].selector = selector;
    idt_entries[num].zero = 0;
    idt_entries[num].gate_attributes = flags | 0x60;
}


// Exeption messages
const char* idt::exception_messages[] = {
    "Devide Error (#DE)",
    "Debug Exception (#DB)",
    "NMI Interrupt",
    "Breakpoint (#BP)",
    "Overflow (#OF)",
    "BOUND Range Exceeded (#BR)",
    "Invalid Opcode (Undefined Opcode) (#UD)",
    "Device Not Available (No Math Coprocessor) (#NM)",
    "Double Fault (#DF)",
    "Coprocessor Segment Overrun",
    "Invalid TSS (#TS)",
    "Segment Not Present (#NP)",
    "Stack-Segment Fault (#SS)",
    "General Protection (#GP)",
    "Page Fault (#PF)",
    "Reserved",
    "x87 FPU Floating-Point Error (Math Fault) (#MF)",
    "Alignment Check (#AC)",
    "Machine Check (#MC)",
    "SIMD Floating-Point Exception (#XM)",
    "Virtualization Exception (#VE)",
    "Control Protection Exception (#CP)",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};


// Interrupt Service Routine error message
extern "C" void isr_handler(InterruptRegisters* regs) {
    // Throwing kernel panic error
    if(regs->interr_no < 32) {
        kernel_panic(idt::exception_messages[regs->interr_no], regs);
    }
}


#pragma region IRQ Functions


// Array
void* irq_routines[IRQ_QUANTITY] = {
    // Zeroing all
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

// IRQ handler functions

extern "C" void irq_install_handler(int irq_num, void (*handler)(InterruptRegisters* regs)) {
    // Installing IRQ
    irq_routines[irq_num] = (void*)handler;
}

extern "C" void irq_uninstall_handler(int irq_num) {
    // Turrning IRQ back to 0
    irq_routines[irq_num] = 0;
}

// Interrupt request handler
extern "C" void irq_handler(InterruptRegisters* regs) {
    void (*handler)(InterruptRegisters* regs);

    // Getting value from irq_routines
    handler = (void (*)(InterruptRegisters*))irq_routines[regs->interr_no - 32];

    if(handler) {
        handler(regs);
    }

    // EOI signal
    pic::send_eoi(regs->interr_no);
}

#pragma endregion
