// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// pmm_u_test.cpp
// Is in charge of unit testing the PMM
// ========================================

#include <tests/unit_tests.hpp>
#include <mm/vmm.hpp>
#include <mm/pmm.hpp>
#include <interrupts/kernel_panic.hpp>
#include <graphics/vga_print.hpp>

void unittsts::test_vmm(void) {
    #ifdef VMM_HPP
    // If this test is called before initializing the VMM/enabling paging
    if(!vmm::enabled_paging) {
        kprintf(LOG_ERROR, "VMM test: Paging not enabled! Please enable paging!\n");
        return;
    }

    bool passed = true;

    // Testing mapping
    uint32_t address = 0x1000;
    uint32_t phys_addr = (uint32_t)pmm::alloc_frame(1);
    vmm::alloc_page(address, phys_addr, PRESENT | WRITABLE); // Mapping VGA address to 0
    bool is_mapped = vmm::is_mapped(address); // Getting if the page is mapped
    if(!is_mapped) {
        kprintf(LOG_ERROR, "VMM Test 1 failed: couldn't map page at v. address: %x!\n", address);
        passed = false; // Noting that the test failed
    }

    // Testing putting in a value
    uint16_t value = 0x072D;
    uint16_t original_val = *(uint16_t*)(address);
    *(uint16_t*)(address) = value; // Writing in a value
    if(*(uint16_t*)(phys_addr) != value) { // If the value at the physical corresponding address is the same
        kprintf(LOG_ERROR, "VMM Test 2 failed: it set the wrong value (Set: %x Expected: %x Original value: %x)!\n", *(uint32_t*)(phys_addr), value, original_val);
        passed = false;
    }
    *(uint16_t*)(address) = original_val;

    // Testing translation of virtual to physical
    uint64_t phys_address = (uint64_t)vmm::virtual_to_physical(address);
    if(phys_address != phys_addr) {
        kprintf(LOG_ERROR, "VMM Test 3 failed: couldn't translate a virtual address to a physical address!\n");
        passed = false;
    }

    // Testing unmaping
    vmm::free_page(address); // Unmaping
    is_mapped = vmm::is_mapped(address); // Getting if the page is mapped
    if(is_mapped) {
        kprintf(LOG_ERROR, "VMM Test 4 failed: couldn't unmap page at v. address: %x!\n", address);
        passed = false; // Noting that the test failed
    }

    // Testing 4MiB pages
    uint32_t* virt_4mb = (uint32_t*)0x2000000;
    
    vmm::alloc_page_4mib((uint32_t)virt_4mb, (uint32_t)virt_4mb, PRESENT | WRITABLE); // Map 4 MiB page
    
    // Check if virtual address is mapped (naive check: dereference test)
    *virt_4mb = 0xDEADBEEF;

    if (*(uint32_t*)virt_4mb != 0xDEADBEEF) {
        kprintf(LOG_ERROR, "VMM Test 5 failed: 4MiB page did not map correctly!\n");
        passed = false;
    }

    // Freeing page
    vmm::free_page((uint32_t)virt_4mb);
    pmm::free_frame((void*)phys_addr);

    // If this didn't pass al test we'll initialize kernel panic
    if(!passed) kernel_panic("VMM failed!");
    kprintf(LOG_INFO, "Virtual memory manager test passed\n");
    #endif
}
