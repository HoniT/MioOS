// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef HAL_PAGING_HPP
#define HAL_PAGING_HPP

#include <mm/mm_types.hpp>

struct PagingContext {
    PhysAddr value = 0;

    [[nodiscard]] static constexpr PagingContext invalid() noexcept {
        return PagingContext{0};
    }
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return value != 0;
    }

    // Equality is based purely on the physical address value so that
    // AddressSpace can compare contexts without knowing their internals.
    [[nodiscard]] constexpr bool operator==(PagingContext o) const noexcept { return value == o.value; }
    [[nodiscard]] constexpr bool operator!=(PagingContext o) const noexcept { return value != o.value; }
};

/// Paging errors
enum class PagingError : uint8_t {
    Success         = 0,
    OutOfMemory,        // Physical page allocator returned null
    AlreadyMapped,      // A mapping already exists at the target address
    NotMapped,          // No mapping exists at the given address
    InvalidAlignment,   // Address or size is not page-aligned
    InvalidContext,     // PagingContext::is_valid() == false
    InvalidAddress,     // Address outside the valid canonical range
    HardwareFault,      // Architecture-specific hardware error
};

[[nodiscard]] constexpr bool is_ok(PagingError e) noexcept {
    return e == PagingError::Success;
}


/// Virtual HAL interface
class IPagingBackend {
public:
    virtual ~IPagingBackend() = default;

    /// Allocate and initialise a new, empty paging context.
    /// The implementation must copy the kernel's upper-half mappings into
    /// the new context before returning (via share_kernel_mappings()).
    ///
    /// @returns A valid PagingContext on success, PagingContext::invalid()
    ///          if physical memory is exhausted.
    [[nodiscard]] virtual PagingContext create_context() noexcept = 0;

    /// Release all page-table memory owned by `ctx`.
    virtual void destroy_context(PagingContext ctx) noexcept = 0;

    /// Install a single page mapping: virt -> phys with `flags`.
    ///
    /// @param ctx   Target paging context.
    /// @param virt  Page-aligned virtual address (caller must verify).
    /// @param phys  Page-aligned physical address (caller must verify).
    /// @param flags Combination of PageFlags describing permissions/cache.
    ///
    /// @returns Success         — mapping installed.
    ///          AlreadyMapped   — an entry already exists; NOT silently
    ///                            overwritten (caller must unmap first).
    ///          OutOfMemory     — intermediate page table could not be
    ///                            allocated.
    ///          InvalidAlignment— virt or phys not page-aligned.
    [[nodiscard]] virtual PagingError map_page(
        PagingContext ctx,
        VirtAddr      virt,
        PhysAddr      phys,
        PageFlags     flags) noexcept = 0;

    /// Remove the mapping at `virt` in context `ctx`.
    ///
    /// @note  The implementation must issue an appropriate TLB invalidation
    ///        for the unmapped page (invlpg on x86_64).  For bulk unmaps,
    ///        the caller may prefer to batch and call flush_tlb_full().
    ///
    /// @returns Success    — mapping removed and TLB invalidated.
    ///          NotMapped  — no present entry found; no side effects.
    [[nodiscard]] virtual PagingError unmap_page(
        PagingContext ctx,
        VirtAddr      virt) noexcept = 0;

    /// Modify permission flags of an existing mapping in place.
    /// The physical address backing `virt` is unchanged.
    ///
    /// @returns Success   — flags updated and TLB entry invalidated.
    ///          NotMapped — no present entry at `virt`.
    [[nodiscard]] virtual PagingError protect_page(
        PagingContext ctx,
        VirtAddr      virt,
        PageFlags     new_flags) noexcept = 0;


    /// Perform a software page-table walk to translate `virt` to a physical
    /// address within `ctx`.
    ///
    /// @returns Physical address on success, 0 if not mapped or not present.
    /// @note    Does NOT touch the hardware TLB or any hardware register.
    [[nodiscard]] virtual PhysAddr translate(
        PagingContext ctx,
        VirtAddr      virt) const noexcept = 0;

    /// Load `ctx` into the hardware MMU (e.g. write CR3 on x86_64).
    /// @pre  ctx.is_valid() == true.
    virtual void activate(PagingContext ctx) noexcept = 0;

    // TLB Management

    /// Invalidate the TLB entry for a single virtual address.
    /// (Issues `invlpg virt` on x86_64.)
    virtual void flush_tlb_page(VirtAddr virt) noexcept = 0;

    /// Flush ALL non-global TLB entries.
    /// On x86_64 this is achieved by reloading CR3 with the current value.
    virtual void flush_tlb_full() noexcept = 0;

    /// Propagate kernel upper-half entries from the kernel root context into
    /// `ctx`.  Called by create_context() and must also be invoked whenever
    /// a new kernel mapping is installed that must be visible to all tasks.
    ///
    /// On x86_64 (4-level paging), this copies PML4 entries [256..511] from
    /// the kernel's root PML4 into the PML4 of `ctx`.
    virtual void share_kernel_mappings(PagingContext ctx) noexcept = 0;

    /// Return the PagingContext for the kernel's own address space.
    /// This context lives for the entire system lifetime.
    [[nodiscard]] virtual PagingContext kernel_context() const noexcept = 0;
};

#endif // HAL_PAGING_HPP
