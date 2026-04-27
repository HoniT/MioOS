// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VMA_SLAB_HPP
#define VMA_SLAB_HPP

#include <mm/vma.hpp>
#include <mm/mm_types.hpp>
#include <mm/pmm.hpp>

namespace mem
{
    class VMASlabAllocator {
    public:
        static constexpr usize SLOTS_PER_PAGE = PAGE_SIZE / sizeof(VMArea); // 73
        static constexpr usize MAX_SLAB_PAGES = 14; // 14 × 73 = 1022 max VMArea nodes

        /// @param hhdm_base  Same HHDM offset used by the paging backend.
        ///                   Required to dereference physical slab pages as virtual
        ///                   addresses.  Pass 0 for an identity-mapped setup.
        explicit VMASlabAllocator(VirtAddr hhdm_base) noexcept;
        ~VMASlabAllocator() noexcept;

        VMASlabAllocator(const VMASlabAllocator&)            = delete;
        VMASlabAllocator& operator=(const VMASlabAllocator&) = delete;

        /// Allocate one VMArea slot. Returns zero-constructed VMArea* or nullptr.
        [[nodiscard]] VMArea* allocate() noexcept;

        /// Return a VMArea slot to the freelist.
        void deallocate(VMArea* vma) noexcept;

        [[nodiscard]] usize total_slots()     const noexcept;
        [[nodiscard]] usize free_slots()      const noexcept;
        [[nodiscard]] usize allocated_slots() const noexcept;
        [[nodiscard]] usize slab_page_count() const noexcept { return m_page_count; }

    private:
        struct FreeNode { FreeNode* next; };
        static_assert(sizeof(FreeNode)  <= sizeof(VMArea));
        static_assert(alignof(FreeNode) <= alignof(VMArea));

        VirtAddr  m_hhdm_base;
        FreeNode* m_freelist    = nullptr;
        usize    m_page_count  = 0;
        usize    m_free_count  = 0;
        usize    m_total_slots = 0;
        PhysAddr  m_pages[MAX_SLAB_PAGES]{};

        [[nodiscard]] bool expand() noexcept;

        [[nodiscard]] void* phys_to_virt(PhysAddr pa) const noexcept {
            return reinterpret_cast<void*>(m_hhdm_base + pa);
        }
    };

    // Trampolines matching AddressSpace::VMAAllocFn / VMAFreeFn signatures.
    inline VMArea* vma_slab_alloc(void* ctx) noexcept {
        return static_cast<VMASlabAllocator*>(ctx)->allocate();
    }
    inline void vma_slab_free(void* ctx, VMArea* vma) noexcept {
        static_cast<VMASlabAllocator*>(ctx)->deallocate(vma);
    }
} // namespace mem


#endif // VMA_SLAB_HPP
