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

    vga::error("%s\nException! system Halted\n", error);

    for(;;) {}

    return;
}

void kernel_panic(const char* error, InterruptRegistersISR* frame) {
    // (NOTE) Page faults are handled by a page fault handler in vmm.cpp
    asm volatile ("cli"); // Clearing interrupts

    uint32_t _cr3, ss;
    asm volatile ("mov %%cr3, %0" : "=r"(_cr3));
    asm volatile ("mov %%ss, %0" : "=r"(ss));
    vga::error("CR3: %x, SS: %x\n", _cr3, ss);

    vga::error("%s\nException! System halted\n", error);
    // Printing info
    vga::error("DS: %x\n", frame->ds);
    vga::error("EDI: %x\n", frame->edi);
    vga::error("ESI: %x\n", frame->err_code);
    vga::error("EBP: %x\n", frame->err_code);
    vga::error("ESP: %x\n", frame->err_code);
    vga::error("EBX: %x\n", frame->err_code);
    vga::error("EDX: %x\n", frame->err_code);
    vga::error("ECX: %x\n", frame->err_code);
    vga::error("EAX: %x\n", frame->err_code);
    vga::error("Interrupt Number: %x\n", frame->interr_no);
    vga::error("Error code: %x\n", frame->err_code);
    vga::error("EIP: %x\n", frame->eip);
    vga::error("CS: %x\n", frame->cs);
    vga::error("EFlags: %x\n", frame->eflags);
    vga::error("User SP: %x\n", frame->useresp);
    vga::error("SS: %x\n", frame->ss);

    // Printing major Control Registers
    uint32_t cr0, cr3, cr4;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    vga::error("CR0: %x\n", cr0);
    vga::error("CR3: %x\n", cr3);
    vga::error("CR4: %x\n", cr4);

    for(;;) {}

    return;
}
