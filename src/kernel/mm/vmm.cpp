// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// vmm.cpp
// Is the Virtual Memory Manager, takes care of paging / pages
// ========================================

#include <mm/vmm.hpp>
#include <mm/pmm.hpp>
#include <drivers/vga_print.hpp>
#include <cpuid.hpp>
#include <interrupts/idt.hpp>
#include <interrupts/kernel_panic.hpp>
#include <lib/mem_util.hpp>

bool enabled_paging = false; // Stores paging enabled status
bool pae_enabled = false; // Stores if paging has been enabled with the PAE

// Function declarations
void page_fault_handler(InterruptFrame* frame);

// Initializes the VMM, sets PD, PDPT, PT structures
void vmm::init(void) {
    // Setting ISR 14 to the Page Fault Handler
    idt::set_idt_gate(14, uint32_t(page_fault_handler), 0x08, 0x8E);

    // Checking if the CPU supports the PAE (Physical Address Extension)
    // Getting feature list
    CPUIDResult cpuid_res = cpu::cpuid(CPUID_FEATURES);

    // Checking if bit 6 of EDX is set
    if(cpuid_res.edx & (1 << 6)) {
        vga::printf("PAE is supported!\n");
        goto enable_36bit;
    } else {
        vga::printf("PAE is not supported!\n");
        goto enable_32bit;
    }

enable_32bit:
    // For now we'll just kernel panic, may add support for non PAE paging in future
    kernel_panic("Physical Address Extension not supported on CPU!\n");
    
    vga::printf("Implemented VMM with 32 bit paging!\n");
    return;

enable_36bit:
    // Setting the paging structures
    // Allocating memory for PDPT
    pdpt_t* pdpt = (pdpt_t*)pmm::alloc_frame_aligned(1);
    memset(pdpt, 0, sizeof(pdpt_t));

    for (uint8_t i = 0; i < PDPT_ENTRIES; i++) {
        pd_t* pd = (pd_t*)pmm::alloc_frame_aligned(1); // Allocate PD
        memset(pd, 0, sizeof(pd_t));
        pdpt->entries[i] = pd; // Link PD to PDPT
    
        for (uint16_t j = 0; j < PD_ENTRIES; j++) {
            pt_t* pt = (pt_t*)pmm::alloc_frame_aligned(1); // Allocate PT
            memset(pt, 0, sizeof(pt_t));
            pd->entries[j] = pt; // Link PT to PD
        }
    }

    for (int i = 0; i < PDPT_ENTRIES; i++) {
        pd_t* pd = pdpt->entries[i];
        
        for (int j = 0; j < PD_ENTRIES; j++) {
            pt_t* pt = pd->entries[j];
            
            for (int k = 0; k < PT_ENTRIES; k++) {
                uint64_t phys_addr = ((i * PD_ENTRIES + j) * PT_ENTRIES + k) * 0x1000; // 4KB pages
                
                pt->entries[k].present = 1;
                pt->entries[k].rw = 1;
                pt->entries[k].user = 0; // Supervisor mode
                pt->entries[k].frame = (phys_addr & 0xFFFFFFFFFFFFF000) >> 12;

            }
        }
    }
    
    set_pdpt(uint64_t(pdpt));
    enable_pae();
    // enable_paging();

    flush_tlb(uint64_t(pdpt));

    vga::printf("Implemented VMM with 36 bit paging!\n");
    pae_enabled = true;
    return;
    
}


// This will handle page faults and print needed info for debugging
void page_fault_handler(InterruptFrame* frame) {
    // Getting faulting address from Control Register 2
    uint32_t faulting_address;
    // Read the CR2 register to get the faulting address
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

    // Get the error code from the interrupt frame
    uint32_t errorCode = frame->errorCode;

    // Decode the error code
    bool present = errorCode & PRESENT;       // Page not present
    bool write = errorCode & WRITABLE;        // Write operation
    bool userMode = errorCode & USER;         // User-mode access
    bool reservedWrite = errorCode & WRITETHROUGH;    // Overwrite of a reserved bit
    bool instructionFetch = errorCode & NOTCACHABLE;  // Instruction fetch

    // Display error details
    vga::print_set_color(PRINT_COLOR_BLUE, PRINT_COLOR_WHITE);
    vga::printf("Page Fault Detected!\n");
    vga::printf("Faulting Address: %x\n", faulting_address);
    vga::printf("Error Code: %x\n", errorCode);
    vga::printf("Details: [");

    if (present) vga::printf(" PRESENT ");
    if (write) vga::printf(" WRITE ");
    if (userMode) vga::printf(" USER ");
    if (reservedWrite) vga::printf(" RESERVED_WRITE ");
    if (instructionFetch) vga::printf(" INSTRUCTION_FETCH ");

    vga::printf("]\n");

    // Halt the system
    kernel_panic("Page Fault occurred. System halted.\n");
}
