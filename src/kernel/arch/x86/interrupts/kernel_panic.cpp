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

void kernel_panic(const char* error, InterruptRegisters* frame) {
    // (NOTE) Page faults are handled by a page fault handler in vmm.cpp
    asm volatile ("cli"); // Clearing interrupts

    vga::error("%s\nException! System halted\n", error);
    // Printing info
    
    vga::error("DS: %x\n", frame->ds);
    vga::error("EDI: %x ", frame->edi);
    vga::error("ESI: %x ", frame->esi);
    vga::error("EBP: %x ", frame->ebp);
    vga::error("ESP: %x\n", frame->manual_esp);
    vga::error("EBX: %x ", frame->ebx);
    vga::error("EDX: %x ", frame->edx);
    vga::error("ECX: %x ", frame->ecx);
    vga::error("EAX: %x\n", frame->eax);
    vga::error("Interrupt Number: %x\n", frame->interr_no);
    vga::error("Error code: %x\n", frame->err_code);
    vga::error("EIP: %x\n", frame->eip);
    vga::error("CS: %x\n", frame->cs);
    vga::error("EFlags: %x\n", frame->eflags);
    vga::error("ESP (Pushed by CPU): %x\n", frame->esp);
    vga::error("SS: %x\n", frame->ss);

    // Printing major Control Registers
    uint32_t cr0, cr3, cr4;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    vga::error("CR0: %x\n", cr0);
    vga::error("CR3: %x\n", cr3);
    vga::error("CR4: %x\n", cr4);

    // If it is a page fault
    if(frame->interr_no == 14) {
        if(frame->interr_no == 14)
            vga::error("CR2 (Page fault address): %x\n", frame->cr2);

        // Get the error code from the interrupt frame
        uint32_t errorCode = frame->err_code;

        // Decode the error code
        bool present = errorCode & 0x1;       // Page not present
        bool write = errorCode & 0x2;        // Write operation
        bool userMode = errorCode & 0x4;         // User-mode access
        bool reservedWrite = errorCode & 0x8;    // Overwrite of a reserved bit
        bool instructionFetch = errorCode & 0x10;  // Instruction fetch

        vga::error("Details for Page fault (#PF): [");

        if (present) vga::error(" PRESENT ");
        if (write) vga::error(" WRITE ");
        if (userMode) vga::error(" USER ");
        if (reservedWrite) vga::error(" RESERVED_WRITE ");
        if (instructionFetch) vga::error(" INSTRUCTION_FETCH ");

        vga::error("]\n");
    }

    for(;;) {}

    return;
}
