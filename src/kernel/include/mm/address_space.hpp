// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef ADDRESS_SPACE_HPP
#define ADDRESS_SPACE_HPP

#include <mm/mm_types.hpp>
#include <mm/hal_paging.hpp>
#include <mm/vma.hpp>

using mem::VMArea;
using mem::VMATree;

struct MapResult {
    PagingError error       = PagingError::Success;
    VirtAddr    mapped_addr = 0;  // The virtual address at which mapping was placed

    [[nodiscard]] constexpr bool ok() const noexcept {
        return error == PagingError::Success;
    }

    [[nodiscard]] static constexpr MapResult ok(VirtAddr addr) noexcept {
        return MapResult{PagingError::Success, addr};
    }
    [[nodiscard]] static constexpr MapResult fail(PagingError e) noexcept {
        return MapResult{e, 0};
    }
};

/// The main address space class
class AddressSpace {
public:
    // Canonical split between user space and kernel space (48-bit VA)
    static constexpr VirtAddr USER_START    = 0x0000'0000'0001'0000ULL;
    static constexpr VirtAddr USER_END      = 0x0000'7FFF'FFFF'F000ULL;
    static constexpr VirtAddr KERNEL_START  = KERNEL_VIRT_BASE;

    // Callbacks for allocating/freeing VMA nodes without coupling to a specific allocator
    using VMAAllocFn = VMArea* (*)(void* allocator_ctx) noexcept;
    using VMAFreeFn  = void    (*)(void* allocator_ctx, VMArea*) noexcept;

    // Creates an AddressSpace backed by the given HAL paging backend
    AddressSpace(IPagingBackend* backend,
                 VMAAllocFn     vma_alloc,
                 VMAFreeFn      vma_free,
                 void* allocator_ctx,
                 bool           is_kernel = false) noexcept;

    // Destroys the address space, freeing all VMAs and hardware contexts
    ~AddressSpace() noexcept;

    // Non-copyable — address spaces are unique hardware resources
    AddressSpace(const AddressSpace&)            = delete;
    AddressSpace& operator=(const AddressSpace&) = delete;

    // Maps physical memory to a virtual address range
    [[nodiscard]] MapResult map(VirtAddr  virt,
                                PhysAddr  phys,
                                usize     size,
                                PageFlags flags,
                                VMAType   type = VMAType::Anonymous) noexcept;

    [[nodiscard]] PagingError register_vma(VirtAddr  virt,
                                           usize     size,
                                           PageFlags flags,
                                           VMAType   type = VMAType::Kernel) noexcept;

    // Unmaps and frees a previously mapped virtual address range
    [[nodiscard]] PagingError unmap(VirtAddr virt, usize size) noexcept;

    // Changes permission/cache flags for an existing mapping
    [[nodiscard]] PagingError protect(VirtAddr  virt,
                                      usize     size,
                                      PageFlags new_flags) noexcept;

    // Resolves a virtual address to its physical backing via the hardware tables
    [[nodiscard]] PhysAddr translate(VirtAddr virt) const noexcept;

    // Checks if an entire range is mapped and contains the required flags
    [[nodiscard]] bool is_mapped(VirtAddr  virt,
                                 usize     size,
                                 PageFlags required_flags) const noexcept;

    // Finds the specific VMA containing, or overlapping, the given address(es)
    [[nodiscard]] VMArea* find_vma(VirtAddr addr) const noexcept;
    [[nodiscard]] VMArea* find_overlap(VirtAddr addr, usize size) const noexcept;

    // Finds an unmapped virtual region of the requested size
    [[nodiscard]] VirtAddr find_free_region(VirtAddr hint, usize size) const noexcept;

    // Switches the hardware MMU (e.g., writes to CR3) to this address space
    void activate() noexcept;

    // Exposes the underlying hardware context handle
    [[nodiscard]] PagingContext context() const noexcept { return m_context; }

    // Iterates over all VMAs in ascending memory order
    template<typename Fn>
    void for_each_vma(Fn&& fn) const noexcept {
        m_vma_tree.for_each(fn);
    }

    [[nodiscard]] usize vma_count()       const noexcept { return m_vma_tree.count(); }
    [[nodiscard]] usize mapped_bytes()    const noexcept { return m_mapped_bytes; }
    [[nodiscard]] bool  is_kernel_space() const noexcept { return m_is_kernel; }

private:
    IPagingBackend* m_backend;           // HAL backend
    PagingContext   m_context;           // Hardware context (e.g., CR3 physical address)
    VMATree         m_vma_tree;          // Sorted logical regions
    usize           m_mapped_bytes = 0;  // Total mapped memory footprint
    bool            m_is_kernel    = false;

    VMAAllocFn  m_vma_alloc;
    VMAFreeFn   m_vma_free;
    void* m_allocator_ctx;

    // Helper functions to allocate/free VMAs using the injected callbacks
    [[nodiscard]] VMArea* alloc_vma() noexcept;
    void free_vma(VMArea* vma) noexcept;

    // Unmaps/maps individual hardware pages directly (does not touch the VMA tree)
    void unmap_pages(VirtAddr start, usize size) noexcept;
    [[nodiscard]] PagingError map_pages(VirtAddr  virt,
                                        PhysAddr  phys,
                                        usize     size,
                                        PageFlags flags) noexcept;
};

#endif //ADDRESS_SPACE_HPP
