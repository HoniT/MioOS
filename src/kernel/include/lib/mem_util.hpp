// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef MEM_UTIL_HPP
#define MEM_UTIL_HPP

#include <stdint.h>
#include <stddef.h>

// Memory utility functions
void memset(const void *dest, const char val, uint32_t count);
void* memcpy(void* dest, const void* src, size_t n);
size_t align_up(const size_t value, const size_t alignment);

#endif // MEM_UTIL_HPP
