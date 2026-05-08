// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef MM_DEFS_HPP
#define MM_DEFS_HPP

#include <stdint.h>

namespace mem
{
    using PhysAddr = uint64_t;
    using VirtAddr = uint64_t;

    constexpr PhysAddr HIGHER_HALF_OFFSET = 0xFFFFFFFF80000000;
} // namespace mem

#endif // MM_DEFS_HPP
