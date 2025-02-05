// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// vmm.cpp
// Is the Virtual Memory Manager, takes care of paging / pages
// ========================================

#include <mm/vmm.hpp>
#include <mm/pmm.hpp>
#include <lib/mem_util.hpp>
#include <drivers/vga_print.hpp>
#include <interrupts/kernel_panic.hpp>
#include <interrupts/idt.hpp>

// If this is true we'll map a page in the PMM
bool enabled_paging = false;

PageDirectory* current_page_directory = nullptr;

namespace vmm {
    // Constants for higher-half kernel mapping
    constexpr uint32_t KERNEL_BASE = 0xC0000000; // Higher-half kernel base address
    constexpr uint32_t KERNEL_PHYS_BASE = 0x00100000; // Physical address where the kernel is loaded

    // VMM functions

    // Flushes TLB
    void flush_tlb(const uint32_t virtAddr) {
        asm volatile("invlpg (%0)" ::"r"(virtAddr) : "memory");
    }

    // Sets the page directory
    void set_pd(PageDirectory* pageDirectory) {
        current_page_directory = pageDirectory;

        // Load Page Directory into CR3
        set_page_directory(current_page_directory);
    }

    // Initializes the VMM
    void init() {
        // Switching ISR 14 to page fault handler
        idt::set_idt_gate(14, uint32_t(vmm::page_fault_handler), 0x08, 0x8E);

        // Create a default page directory
        PageDirectory* pageDirectory = (PageDirectory*)pmm::alloc_lframe(3);

        if (!pageDirectory) {
            kernel_panic("Out of memory! Can't allocate PD!\n");
            return; // Out of memory
        }

        // Clear page directory and set as current
        memset(pageDirectory, 0, sizeof(PageDirectory));
        // Identity map the page directory itself
        pd_ent* pageDirectoryEntry = &pageDirectory->entries[PD_INDEX((uint32_t)pageDirectory)];
        SET_ATTRIBUTE(pageDirectoryEntry, PDE_PRESENT | PDE_WRITABLE);
        SET_FRAME(pageDirectoryEntry, (uint32_t)pageDirectory);

        // Allocate page table for 0-4MB
        PageTable* table = (PageTable *)pmm::alloc_lframe(1);

        if (!table) {
            kernel_panic("Out of memory! Can't allocate PT!\n");
            return; // Out of memory
        }

        // Allocate a 3GB page table
        PageTable* table3G = (PageTable *)pmm::alloc_lframe(1);

        if (!table3G) {
            kernel_panic("Out of memory! Can't allocate 3G PT!\n");
            return; // Out of memory
        }

        // Allocate a page table for 0xF0000000-0xFFFFFFFF
        PageTable* tableF = (PageTable *)pmm::alloc_lframe(1);

        if (!tableF) {
            kernel_panic("Out of memory! Can't allocate 0xF PT!\n");
            return; // Out of memory
        }

        // Clear page tables
        memset(table, 0, sizeof(PageTable));
        memset(table3G, 0, sizeof(PageTable));
        memset(tableF, 0, sizeof(PageTable));

        // Identity map 1st 4MiB of memory
        for (uint32_t i = 0, frame = 0x0, virt = 0x0; i < 1024; i++, frame += PAGE_SIZE, virt += PAGE_SIZE) {
            // Create new page
            pt_ent page = 0;
            SET_ATTRIBUTE(&page, PTE_PRESENT);
            SET_ATTRIBUTE(&page, PTE_WRITABLE);
            SET_FRAME(&page, frame);

            // Add page to 3GB page table
            table3G->entries[PT_INDEX(virt)] = page;
        }

        // Map kernel to 3GiB+ addresses (higher half kernel)
        for (uint32_t i = 0, frame = KERNEL_PHYS_BASE, virt = 0xC0000000; i < 1024; i++, frame += PAGE_SIZE, virt += PAGE_SIZE) {
            // Create new page
            pt_ent page = 0;
            SET_ATTRIBUTE(&page, PTE_PRESENT);
            SET_ATTRIBUTE(&page, PTE_WRITABLE);
            SET_FRAME(&page, frame);

            // Add page to 0-4MB page table
            table->entries[PT_INDEX(virt)] = page;
        }

        // Identity map 0xF0000000-0xFFFFFFFF
        for (uint32_t i = 0, frame = 0xF0000000, virt = 0xF0000000; i < 1024; i++, frame += PAGE_SIZE, virt += PAGE_SIZE) {
            // Create new page
            pt_ent page = 0;
            SET_ATTRIBUTE(&page, PTE_PRESENT);
            SET_ATTRIBUTE(&page, PTE_WRITABLE);
            SET_FRAME(&page, frame);

            // Add page to the 0xF0000000 page table
            tableF->entries[PT_INDEX(virt)] = page;
        }

        // Set up the page directory entries
        pd_ent* entry = &pageDirectory->entries[PD_INDEX(0xC0000000)];
        SET_ATTRIBUTE(entry, PDE_PRESENT);
        SET_ATTRIBUTE(entry, PDE_WRITABLE);
        SET_FRAME(entry, uint32_t(table)); // 3GB directory entry points to default page table

        pd_ent* entry2 = &pageDirectory->entries[PD_INDEX(0x00000000)];
        SET_ATTRIBUTE(entry2, PDE_PRESENT);
        SET_ATTRIBUTE(entry2, PDE_WRITABLE);   
        SET_FRAME(entry2, uint32_t(table3G));    // Default dir entry points to 3GB page table

        pd_ent* entryF = &pageDirectory->entries[PD_INDEX(0xF0000000)];
        SET_ATTRIBUTE(entryF, PDE_PRESENT);
        SET_ATTRIBUTE(entryF, PDE_WRITABLE);
        SET_FRAME(entryF, uint32_t(tableF));    // Entry for 0xF0000000 points to page table

        // Switch to page directory
        set_page_directory(pageDirectory);

        // Enable paging: Set PG (paging) bit 31 and PE (protection enable) bit 0 of CR0
        enable_paging();
        enabled_paging = true;

        vga::printf("VMM initialized with 0xF0000000-0xFFFFFFFF identity-mapped!\n");
    }


    void map_page(const uint32_t virtAddr, const uint32_t physAddr, uint32_t flags) {
        // Debugging
        vga::printf("---------------------Mapping Page\n");
        // Getting page directory
        PageDirectory* pageDirectory = current_page_directory;

        // Getting page table
        pd_ent* entry = &(pageDirectory->entries[PD_INDEX(virtAddr)]);

        /* If this entries present flag doesn't equal PDE_PRESENT
         * Basicaly if it is not present */
        if((*entry & PDE_PRESENT) != PDE_PRESENT) {
            // Debugging
            vga::error("PDE not present!\n");
            // Allocating the page table
            PageTable* pageTable = (PageTable*)pmm::alloc_lframe(1);
            if (!pageTable) {
                kernel_panic("Out of memory! Couldn't allocate PT!");
                return;
            }

            // Clearing page table
            memset(pageTable, 0, sizeof(PageTable));

            // Creating new entry for page directory
            pd_ent* _entry = &(pageDirectory->entries[PD_INDEX(virtAddr)]);

            // Map in table and enable attributes
            SET_ATTRIBUTE(_entry, PDE_PRESENT | PDE_WRITABLE);
            SET_FRAME(_entry, uint32_t(pageTable));
        }

        // Else if it is present

        // Get table
        PageTable* pageTable = (PageTable*)(PAGE_PHYS_ADDRESS(entry));

        // Get page in the table
        pt_ent* page = &(pageTable->entries[PT_INDEX(virtAddr)]);

        // Map in page
        SET_ATTRIBUTE(page, flags);
        SET_FRAME(page, physAddr);

        vmm::flush_tlb(virtAddr);
        // Debugging
        vga::printf("---------------------Ended mapping Page with flags %x\n", flags);
    }

    void unmap_page(const uint32_t virtAddr) {
        pt_ent* page = get_page(virtAddr);

        SET_FRAME(page, 0); // Sets physical address to 0
        CLEAR_ATTRIBUTE(page, PTE_PRESENT); // Making page not present

        vmm::flush_tlb(virtAddr);
    }

    void* allocate_page(pt_ent* page) {
        uint32_t frame = uint32_t(pmm::alloc_lframe(1));

        if(frame != -1) {
            // Map page to frame
            SET_FRAME(page, frame);
            SET_ATTRIBUTE(page, PTE_PRESENT);
        }

        flush_tlb(uint32_t(page));

        return (void*)(frame);
    }

    void free_page(pt_ent* page) {
        void* address = (void*)(PAGE_PHYS_ADDRESS(page));
        if(address) pmm::free_frame(address);

        CLEAR_ATTRIBUTE(page, PTE_PRESENT); // Setting to not present

        flush_tlb(uint32_t(page));
    }

    pt_ent* get_page(const uint32_t address)
    {
        // Get page directory
        PageDirectory* pd = current_page_directory; 

        // Get page table in directory
        pd_ent* entry = &(pd->entries[PD_INDEX(address)]);
        PageTable* table = (PageTable*)PAGE_PHYS_ADDRESS(entry);

        // Get page in table
        pt_ent* page = &(table->entries[PT_INDEX(address)]);
        
        // Return page
        return page;
    }

    pd_ent* get_pd_ent(const PageDirectory* pd, const uint32_t virtAddr) {
        return (pd) ? (pd_ent*)&(pd->entries[PD_INDEX(virtAddr)]) : 0;
    }

    pt_ent* get_pt_ent(const PageTable* pt, const uint32_t virtAddr) {
        return (pt) ? (pt_ent*)&(pt->entries[PT_INDEX(virtAddr)]) : 0;
    }


    bool is_mapped(const void* virtAddr) {
        PageDirectory* pageDirectory = current_page_directory;

        // Get the PDE from the Page Directory
        pd_ent* pdEntry = &(pageDirectory->entries[PD_INDEX((uint32_t)virtAddr)]);

        // Check if the Page Directory entry is present
        if (!(*pdEntry & PDE_PRESENT)) {
            vga::error("Page Table is not present!\n");
            return false; // Page Table is not present
        }

        // Get the Page Table from the PDE
        PageTable* pageTable = (PageTable*)(PAGE_PHYS_ADDRESS(pdEntry));

        // Get the PTE from the Page Table
        pt_ent* ptEntry = &(pageTable->entries[PT_INDEX((uint32_t)virtAddr)]);

        // Check if the Page Table entry is present
        if (!(*ptEntry & PTE_PRESENT)) {
            vga::error("Page is not present!\n");
            return false; // Page is not present
        }

        // The page is successfully mapped
        return true;
    }

    void test_page_mapping(const void* virtAddr) {
        if (is_mapped(virtAddr)) {
            // The page is mapped, we can safely use it
            uint8_t* testAddr = (uint8_t*)virtAddr;
            *testAddr = 0xAB;  // Write some test data
            uint8_t value = *testAddr;  // Read it back
            vga::printf("Successfully wrote/read back value from virtual address %x: %x\n", virtAddr, value);
        } else {
            vga::error("Page is not mapped! Cannot access it!\n");
        }
    }

    uint32_t translate_virtual_to_physical(const uint32_t virtAddr) {
        // Get the Page Directory Entry
        pd_ent* pdEntry = get_pd_ent(current_page_directory, virtAddr);
        if (!TEST_ATTRIBUTE(pdEntry, PDE_PRESENT)) {
            return 0; // Page directory entry not present, page fault
        }

        // Get the Page Table
        PageTable* pageTable = reinterpret_cast<PageTable*>(PAGE_PHYS_ADDRESS(pdEntry));
        pt_ent* ptEntry = get_pt_ent(pageTable, virtAddr);
        if (!TEST_ATTRIBUTE(ptEntry, PTE_PRESENT)) {
            return 0; // Page table entry not present, page fault
        }

        // Extract the physical frame address from the Page Table Entry
        uint32_t physFrame = PAGE_PHYS_ADDRESS(ptEntry);

        // Combine with the page offset to get the full physical address
        uint32_t pageOffset = virtAddr & (PAGE_SIZE - 1);
        return physFrame | pageOffset;
    }

    void page_fault_handler(InterruptFrame* frame) {
        uint32_t faultingAddress;

        // Read the CR2 register to get the faulting address
        asm volatile("mov %%cr2, %0" : "=r"(faultingAddress));

        // Get the error code from the interrupt frame
        uint32_t errorCode = frame->errorCode;

        // Decode the error code
        bool present = errorCode & PTE_PRESENT;       // Page not present
        bool write = errorCode & PTE_WRITABLE;        // Write operation
        bool userMode = errorCode & PTE_USER;         // User-mode access
        bool reservedWrite = errorCode & PTE_WRITETHROUGH;    // Overwrite of a reserved bit
        bool instructionFetch = errorCode & PTE_NOTCACHABLE;  // Instruction fetch

        // Display error details
        vga::print_set_color(PRINT_COLOR_BLUE, PRINT_COLOR_WHITE);
        vga::printf("Page Fault Detected!\n");
        vga::printf("Faulting Address: %x\n", faultingAddress);
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


} // namespace vmm
