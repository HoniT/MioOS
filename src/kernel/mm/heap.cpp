// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// heap.cpp
// Is in charge of kernel heap management
// ========================================

#include <mm/heap.hpp>
#include <drivers/vga_print.hpp>
#include <lib/mem_util.hpp>
#include <lib/math.hpp>

// Start of the heap
static HeapBlock* heap_head = nullptr;

// Heap initialization function
void heap::init(void) {
    vga_coords coords = vga::set_init_text("Setting up kernel Heap Memory Manager");
    // Gets the start of the heap
    heap_head = (HeapBlock*)HEAP_START;

    // Setting properties for the heap_head
    /* Setting the size fo the heap_head to be the maximum value,
     * but aswell subtacting the meta data size of HeapBlock */
    heap_head->size = HEAP_SIZE - sizeof(HeapBlock);
    heap_head->free = true;
    heap_head->next = nullptr;

    vga::set_init_text_answer(coords, heap_head && heap_head->free);
}

// Heap memory allocating / deallocating functions
void* kmalloc(const size_t size) {
    if(size <= 0) return nullptr;

    HeapBlock* current = heap_head; // Setting the current block as the head

    // Find a free block with enough space
    while(current) {
        // If the current block is free and has enough space then we can use it
        if(current->free && current->size >= size) {
            // Split the block if theres extra space
            if(current->size > size + sizeof(HeapBlock)) {
                // Setting new block and adding the sizeof the Block to keep metadata in mind
                HeapBlock* new_block = (HeapBlock*)((char*)current + sizeof(HeapBlock) + size);
                // Trimming the extra space of
                new_block->size = current->size - size - sizeof(HeapBlock);
                new_block->free = true;
                /* Setting the current blocks next block as the trimmed (new)
                 * blocks next, because the new block is the end of the current block */
                new_block->next = current->next;
                // Making the current blocks next block the new block
                current->next = new_block;
            }

            current->size = size; // Setting the passed size
            current->free = false; // Mark as allocated

            // Returning the current blocks address plus the metadata size needed
            return (char*)current + sizeof(HeapBlock);
        }
        // Moving to the next block
        current = current->next;
    }

    vga::error("Not enough heap memory for %u bytes!\n", size);
    return nullptr;
}

void kfree(void* ptr) {
    if(!ptr) return;

    // Getting the block based of of the given address/pointer
    HeapBlock* block = (HeapBlock*)((char*)ptr - sizeof(HeapBlock));
    block->free = true; // Setting as free

    // Merging adjacent free blocks to decrease fragmentation
    HeapBlock* current = heap_head; // Setting the current blockto the head
    while(current) {
        // If the current block and the next block are free
        if(current->free && current->next && current->next->free) {
            // Adding the block sizes together
            current->size += current->next->size + sizeof(HeapBlock);
            /* The current blocks next block will be the next blocks next block, 
             * because the next block doesn't exist anymore */
            current->next = current->next->next;

            // Freeing the memory of the deleted node
            kfree(current->next);
        }
        // Going to the next block
        current = current->next;
    }

    /* Setting the ptr to null because it doesn't point to a 
     * valid address anymore and deleting data of of the ptr */
    *(uint32_t*)ptr = 0; 
    ptr = nullptr;
}

// Allocates space for an array
void* kcalloc(const size_t num, const size_t size) {
    // Getting the ptr from malloc
    void* ptr = kmalloc(num * size);
    if(!ptr) {
        vga::error("Out of heap memory!\n");
        return nullptr; // Safety check
    }

    // P.S. malloc already handles memory outages so we won't implement it here

    // Zeroing/Initializing the memory 
    memset(ptr, 0, num * size);

    return ptr;
}

void heap::heap_dump(void) {
    vga::printf("--------------- Heap Dump ---------------\n");

    HeapBlock* current = heap_head; // Setting the current block as the head
    uint64_t bytes_in_use = 0;
    uint32_t allocated_block_num = 0;

    // Itterating through blocks
    while(current) {
        // If the block is free we'll output the size and store add it to the total amount
        vga::printf("Block range: %x-%x\n", current, uint32_t(current) + current->size);
        vga::printf("Block size: %d bytes\n", current->size);
        vga::printf("Block status: %s\n", (current->free) ? "Free" : "Allocated");

        if(!current->free) bytes_in_use += current->size;

        // Getting next block
        current = current->next;
    }

    // Printing every heap block
    vga::printf("\n");

    // Printing final status of heap
    vga::printf("Heap status: %lu bytes used out of %lu (%u%)\n", bytes_in_use, HEAP_SIZE, udiv64(bytes_in_use * 100, HEAP_SIZE));
    vga::printf("-----------------------------------------\n");
}
