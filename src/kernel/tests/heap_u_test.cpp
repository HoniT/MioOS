// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// heap_u_test.cpp
// Is in charge of unit testing the heap manager
// ========================================

#include <tests/unit_tests.hpp>
#include <drivers/vga_print.hpp>
#include <mm/heap.hpp>
#include <interrupts/kernel_panic.hpp>

#ifdef UNIT_TESTS_HPP

// Heap manager unit test
void unittsts::test_heap(void) {
    vga_coords coords = vga::set_init_text("Testing kernel Heap Memory Manager");
    // Final status (passed or failed)
    bool passed = true;


    // Allocating first block
    uint32_t block1 = uint32_t(kmalloc(40));

    if(block1 <= HEAP_START) {
        vga::error("   Test 1 failed: couldn't allocate block in heap!\n");
        passed = false; // Noting that the test failed
    }

    // Allocating second block
    uint32_t block2 = uint32_t(kmalloc(40));
    uint32_t block2_addr = block2;

    // If block2 is equal to block1 plus the difference and plus the metadata size 
    if(block2 != block1 + 40 + sizeof(HeapBlock)) {
        vga::error("   Test 2 failed: couldn't allocate block2 in heap!\n");
        passed = false; // Noting that the test failed
    }

    // Freeing block2
    kfree((void*)block2);

    // Reallocating second block
    block2 = uint32_t(kmalloc(40));

    // If block2 is equal to block1 plus the difference and plus the metadata size 
    if(block2 != block2_addr) {
        vga::error("   Test 3 failed: couldn't free block2!\n");
        passed = false; // Noting that the test failed
    }
    
    // Freeing up heap
    kfree((void*)block1); kfree((void*)block2);

    // If the test failed we will halt the system
    vga::set_init_text_answer(coords, passed);
    if(!passed) kernel_panic("Heap failed!");
}

#endif // UNIT_TEST_HPP
