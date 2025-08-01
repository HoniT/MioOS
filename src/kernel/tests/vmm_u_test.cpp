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
#include <drivers/vga_print.hpp>

void unittsts::test_vmm(void) {
    #ifdef VMM_HPP
    // If this test is called before initializing the VMM/enabling paging
    if(!vmm::enabled_paging) {
        vga::error("Paging not enabled! Please enable paging!\n");
        return;
    }

    bool passed = true;

    // Starter text
    vga::printf("=============== Testing VMM! ================\n");

    // Testing mapping
    uint32_t address = 0x0;
    uint32_t phys_addr = 0xB8000;
    vmm::alloc_page(address, phys_addr, PRESENT | WRITABLE); // Mapping VGA address to 0
    bool is_mapped = vmm::is_mapped(address); // Getting if the page is mapped
    if(is_mapped) 
        vga::printf("   Test 1 successful: mapped page! Mapped v. address: %x\n", address);
    else {
        vga::error("   Test 1 failed: couldn't map page at v. address: %x!\n", address);
        passed = false; // Noting that the test failed
    }

    // Testing putting in a value
    uint16_t value = 0x072D;
    *(uint16_t*)(address + 0xA) = value; // Writing in a value
    if(*(uint16_t*)(phys_addr + 0xA) == value) // If the value at the physical corresponding address is the same
        vga::printf("   Test 2 successful: set value to page! Value: %x\n", value);
    else {
        vga::error("   Test 2 failed: it set the wrong value (%x)!\n", *(uint32_t*)(phys_addr + 0xA));
        passed = false;
    }

    // Testing translation of virtual to physical
    uint64_t phys_address =  (uint64_t)vmm::virtual_to_physical(address);
    if(phys_address == phys_addr) 
        vga::printf("   Test 3 successful: translated a virtual to a physical address!\n");
    else {
        vga::error("   Test 3 failed: couldn't translate a virtual to a physical address!\n");
        passed = false;
    }

    // Testing unmaping
    vmm::free_page(address); // Unmaping
    is_mapped = vmm::is_mapped(address); // Getting if the page is mapped
    if(!is_mapped) 
        vga::printf("   Test 4 successful: unmapped page! Unmapped v. address: %x\n", address);
    else {
        vga::error("   Test 4 failed: couldn't unmap page at v. address: %x!\n", address);
        passed = false; // Noting that the test failed
    }

    // Testing 4MiB pages
    uint32_t* virt_4mb = (uint32_t*)0x2000000;
    
    vmm::alloc_page_4mib((uint32_t)virt_4mb, (uint32_t)virt_4mb, PRESENT | WRITABLE); // Map 4 MiB page
    
    // Check if virtual address is mapped (naive check: dereference test)
    *virt_4mb = 0xDEADBEEF;

    if (*(uint32_t*)virt_4mb == 0xDEADBEEF) {
        vga::printf("   Test 5 successful: 4MiB page mapped and verified!\n");
    } else {
        vga::error("   Test 5 failed: 4MiB page did not map correctly!\n");
        passed = false;
    }

    // Freeing page
    vmm::free_page((uint32_t)virt_4mb);

    // End text
    vga::printf("============= VMM Testing Ended! ============\n");

    // If this didn't pass al test we'll initialize kernel panic
    if(!passed)
        kernel_panic("VMM Failed!");
    #endif
}
