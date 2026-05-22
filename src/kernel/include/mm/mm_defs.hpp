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
    using usize = uint64_t;

    constexpr PhysAddr HIGHER_HALF_OFFSET = 0xFFFFFFFF80000000;
    constexpr VirtAddr HHDM_BASE = 0xFFFF800000000000;

    constexpr usize PAGE_SHIFT   = 12;
    constexpr usize PAGE_SIZE    = 1UL << PAGE_SHIFT;   // 4 KiB
    constexpr usize PAGE_SIZE_2M = 1UL << 21;           // 2 MiB (huge)
    constexpr usize PAGE_SIZE_1G = 1UL << 30;           // 1 GiB (huge)
    constexpr usize PAGE_MASK    = PAGE_SIZE - 1;

    // Page permission flags
    enum class PageFlags : uint32_t {
        None         = 0,
        Read         = (1u << 0),
        Write        = (1u << 1),
        Execute      = (1u << 2),
        User         = (1u << 3),
        Global       = (1u << 4),
        WriteThrough = (1u << 5),
        NoCache      = (1u << 6),
        Huge2M       = (1u << 7),
        Huge1G       = (1u << 8),

        // Convenience presets
        KernelRO = Read,
        KernelRW = Read | Write,
        KernelRX = Read | Execute,
        KernelRWX= Read | Write | Execute,

        UserRO   = Read | User,
        UserRW   = Read | Write | User,
        UserRX   = Read | Execute | User,

        MMIO     = Read | Write | NoCache,
    };

    [[nodiscard]] constexpr PageFlags operator|(PageFlags a, PageFlags b) noexcept {
        return static_cast<PageFlags>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    [[nodiscard]] constexpr PageFlags operator&(PageFlags a, PageFlags b) noexcept {
        return static_cast<PageFlags>(
            static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }
    [[nodiscard]] constexpr PageFlags operator~(PageFlags a) noexcept {
        return static_cast<PageFlags>(~static_cast<uint32_t>(a));
    }
    constexpr PageFlags& operator|=(PageFlags& a, PageFlags b) noexcept {
        a = a | b; return a;
    }
    constexpr PageFlags& operator&=(PageFlags& a, PageFlags b) noexcept {
        a = a & b; return a;
    }

    [[nodiscard]] constexpr bool has_flag(PageFlags flags, PageFlags flag) noexcept {
        return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag))
            == static_cast<uint32_t>(flag);
    }

    /// Round `addr` UP to the next page boundary (or return as-is if aligned).
    [[nodiscard]] constexpr VirtAddr page_align_up(VirtAddr addr) noexcept {
        return (addr + PAGE_MASK) & ~static_cast<VirtAddr>(PAGE_MASK);
    }

    /// Round `addr` DOWN to the nearest page boundary.
    [[nodiscard]] constexpr VirtAddr page_align_down(VirtAddr addr) noexcept {
        return addr & ~static_cast<VirtAddr>(PAGE_MASK);
    }

    /// Return true iff `addr` is aligned to a page boundary.
    [[nodiscard]] constexpr bool is_page_aligned(VirtAddr addr) noexcept {
        return (addr & PAGE_MASK) == 0;
    }

    /// Return true iff `addr` is aligned to `align` bytes (must be a power of 2).
    [[nodiscard]] constexpr bool is_aligned(uintptr_t addr, usize align) noexcept {
        return (addr & (align - 1)) == 0;
    }

    // VMA region types
    enum class VMAType : uint8_t {
        Anonymous,   // Private, zero-initialised anonymous memory
        FileBacked,  // Backed by a file (demand-paged)
        MMIO,        // Memory-mapped I/O — must use NoCache flag
        Stack,       // Thread stack (guard page inserted below by convention)
        Heap,        // Process heap region
        Kernel,      // Kernel-internal region (not accessible from user mode)
        Shared,      // Shared memory between processes
    };

} // namespace mem

#endif // MM_DEFS_HPP
