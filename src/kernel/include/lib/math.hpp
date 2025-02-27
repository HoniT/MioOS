// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#ifndef MATH_HPP
#define MATH_HPP

#include <stdint.h>

#define BYTES_IN_GIB 1073741824

// Math utility functions 

uint32_t hex_to_uint32(const char* hexStr);
uint64_t hex_to_uint64(const char* hexStr);
uint64_t udiv64(uint64_t dividend, uint64_t divisor);
uint64_t umod64(uint64_t dividend, uint64_t divisor);

#endif // MATH_HPP
