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

// Getting the kernels physical starting address from the linker script
extern "C" uint32_t __kernel_phys_base;

// Global variables
bool enabled_paging = false; // Stores paging enabled status
pdpt_t* active_pdpt = nullptr;

// Function declarations
void page_fault_handler(InterruptRegisters* frame);

namespace vmm {
    // Constants addresses for higher-half kernel impl
    constexpr uint32_t KERNEL_BASE = 0xC0000000; // 3GiB higher-half kernel base

    // Initializes the VMM, sets PD, PDPT, PT structures
    void init(void) {
        // Setting ISR 14 to the Page Fault Handler
        idt::set_idt_gate(14, uint32_t(page_fault_handler), 0x08, 0x8E);
        
        bool nx_support = false;

        CPUIDResult cpuid_res_nx = cpu::cpuid(CPUID_EXTENDED_PROCESSOR_INFO);
        // Checking if bit 20 of EDX is set
        if (cpuid_res_nx.edx & (1 << 20)) {
            nx_support = true;
            vga::printf("NX (No-Execute) bit is supported!\n");
        } else {
            vga::printf("NX (No-Execute) bit is NOT supported!\n");
        }

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
        // Allocate memory for PDPT, PDs, and PTs
        alignas(32) pdpt_t* pdpt = (pdpt_t*)pmm::alloc_frame(1);
        alignas(PAGE_SIZE) pd_t* pds = (pd_t*)pmm::alloc_frame(4);
        alignas(PAGE_SIZE) pt_t* pts = (pt_t*)pmm::alloc_frame(2048);

        // Checking if any allocation failed
        if (!pdpt || !pds || !pts) {
            kernel_panic("Paging structures allocation failed!\n");
            return;
        }

        // Testing alignment
        if (!IS_ALIGNED(pdpt, 32))          kernel_panic("PDPT not 32-byte aligned!\n");
        for (int i = 0; i < 4; ++i) {
            if (!IS_ALIGNED(&pds[i], 4096)) kernel_panic("PD not 4KB aligned!\n");
        }
        for (int i = 0; i < 4 * 512; ++i) {
            if (!IS_ALIGNED(&pts[i], 4096)) kernel_panic("PT not 4KB aligned!\n");
        }

        uint64_t frame_addr = 0; // Physical address starts at 0x00000000

        // Set up PDPT
        for (int i = 0; i < 4; i++) {
            pdpt->entries[i] = ((uint64_t)&pds[i] & ~0xFFF) | PRESENT | WRITABLE;
        }

        // Set up PDs and PTs
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 512; j++) {
                int pt_index = (i * 512) + j;
                pds[i].entries[j] = ((uint64_t)&pts[pt_index] & ~0xFFF) | PRESENT | WRITABLE;

                for (int k = 0; k < 512; k++) {
                    pts[pt_index].entries[k] = (frame_addr & ~0xFFF) | PRESENT | WRITABLE;
                    frame_addr += PAGE_SIZE; // Move to next 4KiB frame
                }
            }
        }
        
        // Load PDPT, setting up higher-half kernel, enable PAE, enable paging and flush the TLB
        enable_pae();
        active_pdpt = pdpt;

        // Identity mapping up to 4GiB (or to max RAM installed if 4GiB is not available)
        uint64_t target = (0x100000000 > pmm::total_installed_ram) ? pmm::total_installed_ram : 0x100000000;
        for (uint64_t addr = 0; addr < target; addr += PAGE_SIZE)
            vmm::map_page(addr, addr, PRESENT | WRITABLE);
        
        // Setting up higher half kernel
        // Mapping 0x100000-0x400000 to 3GiB in virtual memory
        uint64_t kernel_base = uint64_t(&__kernel_phys_base);
        for(uint64_t addr = kernel_base, v_addr = KERNEL_BASE; addr < kernel_base + 0x400000; addr += PAGE_SIZE, v_addr += PAGE_SIZE)
            vmm::map_page(v_addr, addr, PRESENT | WRITABLE);

        set_pdpt(uint32_t(pdpt));
        flush_tlb();

        enable_paging();
        flush_tlb();

        enabled_paging = true;
        vga::printf("Paging initialized with identity map up to 4GiB\n");
        
    }

    // Maps an address in virtual memory with given flags
    void map_page(const uint64_t virt_addr, const uint64_t phys_addr, const uint32_t flags) {
        // Retreiving PDPT
        pdpt_t* pdpt = active_pdpt;

        int pdpt_idx = PDPT_INDEX(virt_addr);   // Get PDPT index (2 bits)
        int pd_idx   = PD_INDEX(virt_addr); // Get PD index (9 bits)
        int pt_idx   = PT_INDEX(virt_addr); // Get PT index (9 bits)

        // Get PDPT and PD
        pd_t* pd = (pd_t*)(pdpt->entries[pdpt_idx] & ~0xFFF); 
        // Checking if PD is missing
        if (!pd) {
            kernel_panic("PD not present!\n");
            return;
        }

        // Get or create PT
        pt_t* pt = (pt_t*)(pd->entries[pd_idx] & ~0xFFF);
        if (!(pd->entries[pd_idx] & PRESENT)) { 
            // Allocate new PT
            pt = (pt_t*)pmm::alloc_frame(1);
            // Checking if PT is missing
            if (!pt) {
                kernel_panic("Failed to allocate PT!\n");
                return;
            }
            memset(pt, 0, sizeof(pt_t)); // Clear PT

            // Map new PT in PD
            pd->entries[pd_idx] = ((uint64_t)pt & ~0xFFF) | PRESENT | WRITABLE;
        }

        // Now PT should be valid
        if (!pt) {
            kernel_panic("PT is NULL after allocation!\n");
            return;
        }

        // vga::printf("Mapping %lx -> %lx with flags %x (PT Entry Addr: %lx)\n", virt_addr, phys_addr, flags, (uint64_t)&pt->entries[pt_idx]);

        // Set PT entry
        pt->entries[pt_idx] = (phys_addr & ~0xFFF) | flags | PRESENT;
        
        // Flush TLB
        flush_tlb();
    }

    // Unmaps a page with the given virtual address
    void unmap_page(const uint64_t virt_addr) {
        // Retrieve PDPT
        pdpt_t* pdpt = active_pdpt;
    
        int pdpt_idx = PDPT_INDEX(virt_addr);   // Get PDPT index (2 bits)
        int pd_idx   = PD_INDEX(virt_addr);     // Get PD index (9 bits)
        int pt_idx   = PT_INDEX(virt_addr);     // Get PT index (9 bits)
    
        // Get PD from PDPT
        pd_t* pd = (pd_t*)(pdpt->entries[pdpt_idx] & ~0xFFF);
        if (!pd || !(pdpt->entries[pdpt_idx] & PRESENT)) {
            kernel_panic("ERROR: PD not present!\n");
            return;
        }
    
        // Get PT from PD
        pt_t* pt = (pt_t*)(pd->entries[pd_idx] & ~0xFFF);
        if (!pt || !(pd->entries[pd_idx] & PRESENT)) {
            kernel_panic("ERROR: PT not present!\n");
            return;
        }
    
        // Unmap the page by clearing the entry
        pt->entries[pt_idx] = 0;
    
        // Flush TLB for this address
        flush_tlb();
    
        // Check if the PT is now empty, and free it if necessary
        bool empty = true;
        for (int i = 0; i < 512; i++) {
            if (pt->entries[i] & PRESENT) {
                empty = false;
                break;
            }
        }
    
        if (empty) {
            pmm::free_frame(pt); // Free PT frame
            pd->entries[pd_idx] = 0; // Remove PT entry in PD
        }
    
        // Check if the PD is now empty, and free it if necessary
        empty = true;
        for (int i = 0; i < 512; i++) {
            if (pd->entries[i] & PRESENT) {
                empty = false;
                break;
            }
        }
    
        if (empty) {
            pmm::free_frame(pd); // Free PD frame
            pdpt->entries[pdpt_idx] = 0; // Remove PD entry in PDPT
        }
    
        // vga::printf("Unmapped page at %lx\n", virt_addr);
    }
    
    // Logs and returns a boolean value corresponding if a page at a given address is present/mapped
    bool is_mapped(const uint64_t virt_addr) {
        // Getting PDPT, PD, PT entry indexes based of the given address
        uint64_t pdpt_idx = PDPT_INDEX(virt_addr);
        uint64_t pd_idx   = PD_INDEX(virt_addr);
        uint64_t pt_idx   = PT_INDEX(virt_addr);

        // Retreiving the PDPT and checking id it's present
        pdpt_t* pdpt = active_pdpt;
        if (!(pdpt->entries[pdpt_idx] & PRESENT)) return false;

        // Retreiving the PD and checking id it's present
        pd_t* pd = (pd_t*)(pdpt->entries[pdpt_idx] & ~0xFFF);
        if (!(pd->entries[pd_idx] & PRESENT)) return false;

        // Retreiving the PT and checking id it's present
        pt_t* pt = (pt_t*)(pd->entries[pd_idx] & ~0xFFF);
        if (!(pt->entries[pt_idx] & PRESENT)) return false;

        // Getting if it's present or not and returning
        return pt->entries[pt_idx] & PRESENT;
    }
    
    // Returnes the physical address corresponding to a virtual address
    uint64_t get_physical_address(const uint64_t virt_addr) {
        // Getting PDPT, PD, PT entry indexes based of the given address
        uint64_t pdpt_idx = PDPT_INDEX(virt_addr);
        uint64_t pd_idx   = PD_INDEX(virt_addr);
        uint64_t pt_idx   = PT_INDEX(virt_addr);
    
        // Retreiving the PDPT and checking id it's present
        pdpt_t* pdpt = active_pdpt;
        if (!(pdpt->entries[pdpt_idx] & PRESENT)) return 0xFFFFFFFFFFFFFFFF;
        
        // Retreiving the PD and checking id it's present
        pd_t* pd = (pd_t*)(pdpt->entries[pdpt_idx] & ~0xFFF);
        if (!(pd->entries[pd_idx] & PRESENT)) return 0xFFFFFFFFFFFFFFFF;

        // Retreiving the PT and checking id it's present
        pt_t* pt = (pt_t*)(pd->entries[pd_idx] & ~0xFFF);
        if (!(pt->entries[pt_idx] & PRESENT)) return 0xFFFFFFFFFFFFFFFF;
    
        // Returning the address
        return pt->entries[pt_idx] & ~0xFFF;
    }
    
} // namespace vmm

// This will handle page faults and print needed info for debugging
void page_fault_handler(InterruptRegisters* frame) {
    // Getting faulting address from Control Register 2
    uint32_t faulting_address;
    // Read the CR2 register to get the faulting address
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

    // Get the error code from the interrupt frame
    uint32_t errorCode = frame->err_code;

    // Decode the error code
    bool present = errorCode & PRESENT;       // Page not present
    bool write = errorCode & WRITABLE;        // Write operation
    bool userMode = errorCode & USER;         // User-mode access
    bool reservedWrite = errorCode & WRITETHROUGH;    // Overwrite of a reserved bit
    bool instructionFetch = errorCode & NOTCACHABLE;  // Instruction fetch

    // Display error details
    vga::print_set_color(PRINT_COLOR_BLUE, PRINT_COLOR_WHITE);
    vga::printf("Page Fault Detected!\n");
    vga::printf("Faulting Address: %lx\n", faulting_address);
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
