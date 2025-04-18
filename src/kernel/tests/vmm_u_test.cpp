// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// pmm_u_test.cpp
// Is in charge of unit testing the PMM
// ========================================

#include <tests/unit_tests.hpp>
#include <mm/vmm.hpp>
#include <interrupts/kernel_panic.hpp>
#include <drivers/vga_print.hpp>

void unittsts::test_vmm(void) {
    // If this test is called before initializing the VMM/enabling paging
    if(!enabled_paging) {
        vga::error("Paging not enabled! Please enable paging!\n");
        return;
    }

    bool passed = true;

    // Starter text
    vga::printf("=============== Testing VMM! ================\n");

    // Testing mapping
    uint64_t address = 0x0;
    vmm::map_page(address, 0xB8000, PRESENT | WRITABLE); // Mapping VGA address to 0
    bool is_mapped = vmm::is_mapped(address); // Getting if the page is mapped
    if(is_mapped) 
        vga::printf("   Test 1 successfull: mapped page! Mapped v. address: %lx\n", address);
    else {
        vga::error("   Test 1 failed: couldn't map page at v. address: %lx!\n", address);
        passed = false; // Noting that the test failed
    }

    // Testing putting in a value
    uint16_t value = 0x072D;
    *(uint16_t*)(address + 0xA) = value; // Writing in a value
    if(*(uint16_t*)(0xB8000 + 0xA) == value) // If the value at the physical corresponding address is the same
        vga::printf("   Test 2 successfull: set value to page! Value: %x\n", value);
    else {
        vga::error("   Test 2 failed: it set the wrong value (%x)!\n", *(uint32_t*)(0xB8000 + 0xA));
        passed = false;
    }

    // Testing translation of virtual to physical
    uint64_t phys_address =  vmm::get_physical_address(address);
    if(phys_address == 0xB8000) 
        vga::printf("   Test 3 successfull: translated a virtual to a physical address!\n");
    else {
        vga::error("   Test 3 failed: couldn't translate a virtual to a physical address!\n");
        passed = false;
    }

    // Testing unmaping
    vmm::unmap_page(address); // Unmaping
    is_mapped = vmm::is_mapped(address); // Getting if the page is mapped
    if(!is_mapped) 
        vga::printf("   Test 4 successfull: unmapped page! Unmapped v. address: %lx\n", address);
    else {
        vga::error("   Test 4 failed: couldn't unmap page at v. address: %lx!\n", address);
        passed = false; // Noting that the test failed
    }

    // End text
    vga::printf("============= VMM Testing Ended! ============\n");

    // If this didn't pass al test we'll initialize kernel panic
    if(!passed)
        kernel_panic("VMM Failed!");
}
