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

// Start of the heap
static HeapBlock* heap_head = nullptr;

// Heap initialization function
void heap::init(void) {
    // Gets the start of the heap
    heap_head = (HeapBlock*)HEAP_START;

    // Setting properties for the heap_head
    /* Setting the size fo the heap_head to be the maximum value,
     * but aswell subtacting the meta data size of HeapBlock */
    heap_head->size = HEAP_SIZE - sizeof(HeapBlock);
    heap_head->free = true;
    heap_head->next = nullptr;

    vga::printf("Heap initialized!\n");
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

    vga::error("Not enough heap memory!\n");
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
        }
        // Going to the next block
        current = current->next;
    }

    // Setting the ptr to null because it doesn't point to a valid address anymore
    ptr = nullptr;
}

// Reallocates an already allocated block with a new size
void* krealloc(void* ptr, size_t new_size) {
    if (!ptr) {
        // If the pointer is NULL, it's equivalent to kmalloc
        return kmalloc(new_size);
    }

    // Find the HeapBlock corresponding to ptr
    HeapBlock* block = (HeapBlock*)ptr - 1;  // Move back by the size of HeapBlock

    if (new_size == 0) {
        // If the new size is 0, free the memory
        kfree(ptr);
        return nullptr;
    }

    if (new_size <= block->size) {
        // If the new size is smaller or equal, we shrink the block if necessary
        if (new_size < block->size) {
            // If shrinking, split the block if necessary
            size_t remaining_size = block->size - new_size;
            if (remaining_size >= sizeof(HeapBlock)) {
                // Create a new free block after the current block
                HeapBlock* new_free_block = (HeapBlock*)((char*)block + sizeof(HeapBlock) + new_size);
                new_free_block->size = remaining_size - sizeof(HeapBlock);
                new_free_block->free = true;
                new_free_block->next = block->next;
                block->next = new_free_block;  // Add new block to the linked list
            }
        }
        block->size = new_size;  // Update the size of the block
        return ptr;  // No need to allocate a new block, return the same pointer
    }

    // If the block is not big enough, we need to allocate a new block
    void* new_ptr = kmalloc(new_size);  
    if (!new_ptr) {
        return nullptr;  // Return NULL if allocation fails
    }

    // Copy the data from the old block to the new block
    memcpy(new_ptr, ptr, block->size);

    // Free the old block
    kfree(ptr);

    return new_ptr;
}

// Allocates space for an array
void* kcalloc(const size_t num, const size_t size) {
    // Getting the ptr from malloc
    void* ptr = kmalloc(num * size);
    if(!ptr) {
        vga::error("Out of memory!\n");
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

    // Itterating through blocks
    while(current) {
        if(!current->free) {
            // if the block is free we'll output the size and store add it to the total amount
            vga::printf("Block size: %u bytes\n", current->size);
            bytes_in_use += current->size;
        }

        // Getting next block
        current = current->next;
    }

    // Printing final status of heap
    vga::printf("Heap status: %lu bytes used out of %u\n", bytes_in_use, HEAP_SIZE);
    vga::printf("-----------------------------------------\n");
}
