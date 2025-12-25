// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef MEM_UTIL_HPP
#define MEM_UTIL_HPP

#define BYTES_IN_GIB 1073741824
#define BYTES_IN_MIB 1048576
#define BYTES_IN_KIB 1024

#include <stdint.h>
#include <stddef.h>

namespace data {
    class string;
}

data::string get_units(uint64_t bytes);

// Memory utility functions
void memset(const void *dest, const char val, uint32_t count);
void* memcpy(void* dest, const void* src, size_t n);
uint32_t memcmp(const void *s1, const void *s2, const size_t n);
size_t align_up(const size_t value, const size_t alignment);

#endif // MEM_UTIL_HPP
