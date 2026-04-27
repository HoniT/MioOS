// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef KHEAP_HPP
#define KHEAP_HPP

#include <stddef.h>

inline void* operator new(size_t size, void* ptr) noexcept {
    (void)size;
    return ptr;
}

inline void operator delete(void* ptr, void* place) noexcept {
    (void)ptr;
    (void)place;
}

inline void* operator new[](size_t size, void* ptr) noexcept {
    (void)size;
    return ptr;
}

inline void operator delete[](void* ptr, void* place) noexcept {
    (void)ptr;
    (void)place;
}

inline void operator delete(void* ptr) noexcept {
    (void)ptr;
}

inline void operator delete(void* ptr, size_t size) noexcept {
    (void)ptr;
    (void)size;
}

inline void operator delete[](void* ptr) noexcept {
    (void)ptr;
}

inline void operator delete[](void* ptr, size_t size) noexcept {
    (void)ptr;
    (void)size;
}

#endif // KHEAP_HPP
