// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// pmm_u_test.cpp
// Is in charge of unit testing the PMM
// ========================================

#include <tests/unit_tests.hpp>
#include <mm/pmm.hpp>
#include <drivers/vga_print.hpp>
#include <interrupts/kernel_panic.hpp>

void unittsts::test_pmm(void) {
    // Final status (passed or failed)
    bool passed = true;

    // Starter text
    vga::printf("=============== Testing PMM! ================\n");

    // Allocating first block
    uint32_t* block1 = (uint32_t*)pmm::alloc_lframe(1);

    if(uint32_t(block1) > DATA_LOW_START_ADDR) 
        vga::printf("   Test 1 successfull: allocated frame!\n");
    else {
        vga::error("   Test 1 failed: couldn't allocate frame!\n");
        passed = false; // Noting that the test failed
    }

    // Allocating second block
    uint32_t* block2 = (uint32_t*)pmm::alloc_lframe(1);
    uint32_t block2_addr = uint32_t(block2);

    // If block2 is equal to block1 plus the difference and plus the metadata size 
    if(uint32_t(block2) == uint32_t(block1) + FRAME_SIZE + sizeof(MemoryNode)) 
        vga::printf("   Test 2 successfull: allocated block2!\n");
    else {
        vga::error("   Test 2 failed: couldn't allocate block2!\n");
        passed = false; // Noting that the test failed
    }

    // Freeing block2
    pmm::free_frame(block2);

    // Reallocating second block
    block2 = (uint32_t*)pmm::alloc_lframe(1);

    // If block2 is equal to block1 plus the difference and plus the metadata size 
    if(uint32_t(block2) == block2_addr) 
        vga::printf("   Test 3 successfull: freed block2!\n");
    else {
        vga::error("   Test 3 failed: couldn't free block2!\n");
        passed = false; // Noting that the test failed
    }
    
    // Freeing up memory
    pmm::free_frame(block1); pmm::free_frame(block2);

    // End text
    vga::printf("============= PMM Testing Ended! ============\n");

    // If the test failed we will halt the system
    if(!passed)
        kernel_panic("PMM failed!");
}
