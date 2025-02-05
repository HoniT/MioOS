// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// mem_util.cpp
// In charge of keeping memory utility functions
// ========================================

#include <lib/mem_util.hpp>

// Memset sets a block of memory to a specific value for a given number of bytes
void memset(const void *dest, const char val, uint32_t count){
    char *temp = (char*) dest;
    for (; count != 0; count--){
        *temp++ = val;
    }

}

// Memcpy copies a n sized source to a destination
void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// Alligns the size to the allignment
size_t align_up(const size_t size, const size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}
