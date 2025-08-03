// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// pmm_u_test.cpp
// Is in charge of unit testing the PMM
// ========================================

#include <tests/unit_tests.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <drivers/vga_print.hpp>
#include <interrupts/kernel_panic.hpp>

void unittsts::test_pmm(void) {
    vga_coords coords = vga::set_init_text("Testing Physical Memory Manager");
    // Final status (passed or failed)
    bool passed = true;


    // Allocating first block
    uint32_t block1 = uint32_t(pmm::alloc_frame(1));

    if(block1 <= METADATA_ADDR) {
        vga::error("   Test 1 failed: couldn't allocate frame!\n");
        passed = false; // Noting that the test failed
    }

    // Allocating second block
    uint32_t block2 = uint32_t(pmm::alloc_frame(1));
    uint32_t block2_addr = block2; // Saving the second blocks address

    // If block2 is equal to block1 plus the difference and plus the metadata size 
    if(block2 != block1 + FRAME_SIZE) {
        vga::error("   Test 2 failed: couldn't allocate block2! %x\n", block2);
        passed = false; // Noting that the test failed
    }

    // Freeing block2
    pmm::free_frame((void*)block2);

    // Reallocating second block
    block2 = uint32_t(pmm::alloc_frame(2));

    // If block2 is equal to block1 plus the difference and plus the metadata size 
    if(block2 != block2_addr) { 
        vga::error("   Test 3 failed: couldn't free block2! %x isn't %x\n", block2, block2_addr);
        passed = false; // Noting that the test failed
    }

    // Freeing up memory
    pmm::free_frame((void*)block1); pmm::free_frame((void*)block2);

    // If the test failed we will halt the system
    vga::set_init_text_answer(coords, passed);
    if(!passed) kernel_panic("PMM failed!");
}
