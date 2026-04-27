// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <mm/vma_slab.hpp>

namespace mem
{
    VMASlabAllocator::VMASlabAllocator(VirtAddr hhdm_base) noexcept
        : m_hhdm_base(hhdm_base)
    {
        for (usize i = 0; i < MAX_SLAB_PAGES; ++i)
            m_pages[i] = 0;
    }

    VMASlabAllocator::~VMASlabAllocator() noexcept {
        // Return every backing slab page to the PMM.
        for (usize i = 0; i < m_page_count; ++i) {
            if (m_pages[i]) {
                mem::PMM::free_frame(reinterpret_cast<void*>(m_pages[i]));
                m_pages[i] = 0;
            }
        }
        m_freelist    = nullptr;
        m_page_count  = 0;
        m_free_count  = 0;
        m_total_slots = 0;
    }

    /// Allocate a New Slab Page and Carve It Into Slots
    bool VMASlabAllocator::expand() noexcept {
        if (m_page_count >= MAX_SLAB_PAGES) return false;

        PhysAddr pa = reinterpret_cast<PhysAddr>(mem::PMM::alloc_frame());
        if (!pa) return false;

        m_pages[m_page_count++] = pa;

        auto* base = static_cast<uint8_t*>(phys_to_virt(pa));

        for (usize i = SLOTS_PER_PAGE; i-- > 0; ) {
            auto* node = reinterpret_cast<FreeNode*>(base + i * sizeof(VMArea));
            node->next = m_freelist;
            m_freelist = node;
        }

        m_free_count  += SLOTS_PER_PAGE;
        m_total_slots += SLOTS_PER_PAGE;
        return true;
    }

    VMArea* VMASlabAllocator::allocate() noexcept {
        if (!m_freelist) {
            if (!expand()) return nullptr;
        }

        // Pop the head of the freelist.
        FreeNode* node = m_freelist;
        m_freelist = node->next;
        --m_free_count;

        VMArea* vma = reinterpret_cast<VMArea*>(node);

        auto* raw = reinterpret_cast<uint8_t*>(vma);
        for (usize i = 0; i < sizeof(VMArea); ++i)
            raw[i] = 0;

        vma->avl_height = 1;

        return vma;
    }

    void VMASlabAllocator::deallocate(VMArea* vma) noexcept {
        if (!vma) return;

        auto* node = reinterpret_cast<FreeNode*>(vma);
        node->next = m_freelist;
        m_freelist = node;
        ++m_free_count;
    }

    usize VMASlabAllocator::total_slots()     const noexcept { return m_total_slots; }
    usize VMASlabAllocator::free_slots()      const noexcept { return m_free_count; }
    usize VMASlabAllocator::allocated_slots() const noexcept {
        return m_total_slots - m_free_count;
    }
} // namespace mem
