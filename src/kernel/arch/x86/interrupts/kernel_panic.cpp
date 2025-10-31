// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// kernel_panic.cpp
// In charge of kernel panics
// ========================================

#include <interrupts/kernel_panic.hpp>
#include <graphics/vga_print.hpp>

// Kernel panic manager
void kernel_panic(const char* error) {
    asm volatile ("cli"); // Clearing interrupts

    kprintf(LOG_ERROR, "%s\nException! system Halted\n", error);

    for(;;) {}

    return;
}

void kernel_panic(const char* error, InterruptRegisters* frame) {
    // (NOTE) Page faults are handled by a page fault handler in vmm.cpp
    asm volatile ("cli"); // Clearing interrupts

    kprintf(LOG_ERROR, "%s\nException! System halted\n", error);
    // Printing info
    
    kprintf(STD_PRINT, "DS: %x\n", frame->ds);
    kprintf(STD_PRINT, "EDI: %x ", frame->edi);
    kprintf(STD_PRINT, "ESI: %x ", frame->esi);
    kprintf(STD_PRINT, "EBP: %x ", frame->ebp);
    kprintf(STD_PRINT, "ESP: %x\n", frame->manual_esp);
    kprintf(STD_PRINT, "EBX: %x ", frame->ebx);
    kprintf(STD_PRINT, "EDX: %x ", frame->edx);
    kprintf(STD_PRINT, "ECX: %x ", frame->ecx);
    kprintf(STD_PRINT, "EAX: %x\n", frame->eax);
    kprintf(STD_PRINT, "Interrupt Number: %x\n", frame->interr_no);
    kprintf(STD_PRINT, "Error code: %x\n", frame->err_code);
    kprintf(STD_PRINT, "EIP: %x\n", frame->eip);
    kprintf(STD_PRINT, "CS: %x\n", frame->cs);
    kprintf(STD_PRINT, "EFlags: %x\n", frame->eflags);
    kprintf(STD_PRINT, "ESP (Pushed by CPU): %x\n", frame->esp);
    kprintf(STD_PRINT, "SS: %x\n", frame->ss);

    // Printing major Control Registers
    uint32_t cr0, cr3, cr4;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    kprintf(STD_PRINT, "CR0: %x\n", cr0);
    kprintf(STD_PRINT, "CR3: %x\n", cr3);
    kprintf(STD_PRINT, "CR4: %x\n", cr4);

    // If it is a page fault
    if(frame->interr_no == 14) {
        if(frame->interr_no == 14)
            kprintf(STD_PRINT, "CR2 (Page fault address): %x\n", frame->cr2);

        // Get the error code from the interrupt frame
        uint32_t errorCode = frame->err_code;

        // Decode the error code
        bool present = errorCode & 0x1;       // Page not present
        bool write = errorCode & 0x2;        // Write operation
        bool userMode = errorCode & 0x4;         // User-mode access
        bool reservedWrite = errorCode & 0x8;    // Overwrite of a reserved bit
        bool instructionFetch = errorCode & 0x10;  // Instruction fetch

        kprintf(STD_PRINT, "Details for Page fault (#PF): [");

        if (present) kprintf(STD_PRINT, " PRESENT ");
        if (write) kprintf(STD_PRINT, " WRITE ");
        if (userMode) kprintf(STD_PRINT, " USER ");
        if (reservedWrite) kprintf(STD_PRINT, " RESERVED_WRITE ");
        if (instructionFetch) kprintf(STD_PRINT, " INSTRUCTION_FETCH ");

        kprintf(STD_PRINT, "]\n");
    }

    for(;;) {}

    return;
}
