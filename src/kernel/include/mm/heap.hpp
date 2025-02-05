// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef HEAP_HPP
#define HEAP_HPP

#include <stddef.h>

constexpr size_t HEAP_START = 0x200000; // Heap start (2 MiB mark)
constexpr size_t HEAP_SIZE = 0x300000;  // 3 MiB heap size

// Linked list node
struct HeapBlock {
    size_t size;     // Block size
    bool free;       // Is the block free
    HeapBlock* next; // Next node
};

namespace heap {
    void init(void);
    void heap_dump(void);
} // Namespace heap

// Memory allocation and deallocation functions

// Allocates a block of size <size> to heap and returns address
void* kmalloc(const size_t size);
// Frees a block of memory at pointer <ptr>
void kfree(void* ptr);
// Reallocates an already allocated block with a new size
void* krealloc(void* ptr, const size_t new_size);
// Allocates space for an array
void* kcalloc(const size_t num, const size_t size);

#endif // HEAP_HPP
