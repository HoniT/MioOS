// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Address space logic
// ========================================

#include <mm/address_space.hpp>

using namespace mem;

AddressSpace::AddressSpace(PagingBackend* backend,
                            VMAAllocFn     vma_alloc,
                            VMAFreeFn      vma_free,
                            void*          allocator_ctx,
                            bool           is_kernel) noexcept
    : m_backend      (backend)
    , m_context      (PagingContext::invalid())
    , m_mapped_bytes (0)
    , m_is_kernel    (is_kernel)
    , m_vma_alloc    (vma_alloc)
    , m_vma_free     (vma_free)
    , m_allocator_ctx(allocator_ctx)
{
    if (!backend) return;

    if (is_kernel) {
        // The kernel address space uses the backend's own permanent root
        // context, which was set up during backend initialisation.
        m_context = backend->kernel_context();
    } else {
        // User address space: allocate a fresh hardware context.
        m_context = backend->create_context();
    }
}

AddressSpace::~AddressSpace() noexcept {
    if (!m_backend || !m_context.is_valid()) return;

    while (VMArea* vma = m_vma_tree.root()) {
        unmap_pages(vma->start, vma->size);
        m_vma_tree.remove(vma);
        free_vma(vma);
    }

    // Do NOT destroy the kernel context — it lives forever.
    if (!m_is_kernel) {
        m_backend->destroy_context(m_context);
    }

    m_context = PagingContext::invalid();
}


VMArea* AddressSpace::alloc_vma() noexcept {
    VMArea* vma = m_vma_alloc(m_allocator_ctx);
    if (!vma) return nullptr;

    *vma = VMArea{};
    return vma;
}

void AddressSpace::free_vma(VMArea* vma) noexcept {
    if (vma) m_vma_free(m_allocator_ctx, vma);
}



void AddressSpace::unmap_pages(VirtAddr start, usize size) noexcept {
    const usize page_count = size / PAGE_SIZE;
    for (usize i = 0; i < page_count; ++i) {
        VirtAddr page_va = start + i * PAGE_SIZE;
        m_backend->unmap_page(m_context, page_va);
    }
    m_backend->flush_tlb_full();
}

PagingError AddressSpace::map_pages(VirtAddr  virt,
        PhysAddr  phys,
        usize     size,
        PageFlags flags) noexcept {
    const usize page_count = size / PAGE_SIZE;

    for (usize i = 0; i < page_count; ++i) {
        VirtAddr page_va = virt + i * PAGE_SIZE;
        PhysAddr page_pa = phys + i * PAGE_SIZE;

        PagingError err = m_backend->map_page(m_context, page_va, page_pa, flags);
        if (!is_ok(err)) return err;
    }

    return PagingError::Success;
}


/// Core mapping operation
MapResult AddressSpace::map(VirtAddr  virt,
        PhysAddr  phys,
        usize     size,
        PageFlags flags,
        VMAType   type) noexcept {
    // Input validation

    if (!m_context.is_valid())
        return MapResult::fail(PagingError::InvalidContext);

    if (!is_page_aligned(virt) || !is_page_aligned(phys))
        return MapResult::fail(PagingError::InvalidAlignment);

    if (size == 0 || !is_page_aligned(size))
        return MapResult::fail(PagingError::InvalidAlignment);

    // Overlap check

    if (m_vma_tree.find_overlap(virt, size) != nullptr)
        return MapResult::fail(PagingError::AlreadyMapped);

    // Allocate a VMArea descriptor

    VMArea* vma = alloc_vma();
    if (!vma)
        return MapResult::fail(PagingError::OutOfMemory);

    vma->start = virt;
    vma->size  = size;
    vma->flags = flags;
    vma->type  = type;

    //  Map hardware page table entries

    PagingError err = map_pages(virt, phys, size, flags);
    if (!is_ok(err)) {
        unmap_pages(virt, size);
        free_vma(vma);
        return MapResult::fail(err);
    }

    // Insert into VMA tree

    bool inserted = m_vma_tree.insert(vma);
    (void)inserted;

    // Update statistics

    m_mapped_bytes += size;

    return MapResult::ok(virt);
}


PagingError AddressSpace::register_vma(VirtAddr  virt,
                                        usize     size,
                                        PageFlags flags,
                                        VMAType   type) noexcept {
    if (!is_page_aligned(virt) || !is_page_aligned(size) || size == 0)
        return PagingError::InvalidAlignment;
 
    if (m_vma_tree.find_overlap(virt, size))
        return PagingError::AlreadyMapped;
 
    VMArea* vma = alloc_vma();
    if (!vma) return PagingError::OutOfMemory;
 
    vma->start = virt;
    vma->size  = size;
    vma->flags = flags;
    vma->type  = type;
 
    m_vma_tree.insert(vma);
    m_mapped_bytes += size;
    return PagingError::Success;
}


/// Core unmap operation
PagingError AddressSpace::unmap(VirtAddr virt, usize size) noexcept {
    // Alignment

    if (!is_page_aligned(virt) || !is_page_aligned(size))
        return PagingError::InvalidAlignment;

    if (size == 0) return PagingError::InvalidAlignment;

    if (!m_context.is_valid()) return PagingError::InvalidContext;

    // Lookup

    VMArea* vma = m_vma_tree.find(virt);
    if (!vma)                       return PagingError::NotMapped;
    if (vma->start != virt)         return PagingError::NotMapped;
    if (vma->size  != size)         return PagingError::NotMapped; // Partial unmap not supported

    // Remove from VMA tree

    m_vma_tree.remove(vma);
    m_mapped_bytes -= vma->size;

    // Unmap hardware PTEs

    unmap_pages(virt, size);

    // Free VMA node

    free_vma(vma);

    return PagingError::Success;
}

// Changes page perms
PagingError AddressSpace::protect(VirtAddr  virt,
                                   usize     size,
                                   PageFlags new_flags) noexcept {
    if (!is_page_aligned(virt) || !is_page_aligned(size))
        return PagingError::InvalidAlignment;

    if (size == 0)              return PagingError::InvalidAlignment;
    if (!m_context.is_valid())  return PagingError::InvalidContext;

    // Find and validate the VMA.
    VMArea* vma = m_vma_tree.find(virt);
    if (!vma)               return PagingError::NotMapped;
    if (vma->start != virt) return PagingError::NotMapped;
    if (vma->size  != size) return PagingError::NotMapped;

    const usize page_count = size / PAGE_SIZE;

    // Update hardware entries first.
    for (usize i = 0; i < page_count; ++i) {
        VirtAddr page_va = virt + i * PAGE_SIZE;
        PagingError err = m_backend->protect_page(m_context, page_va, new_flags);
        if (!is_ok(err)) {
            return err;
        }
    }

    vma->flags = new_flags;
    return PagingError::Success;
}

// Virt to phis
PhysAddr AddressSpace::translate(VirtAddr virt) const noexcept {
    if (!m_context.is_valid()) return 0;
    return m_backend->translate(m_context, virt);
}


bool AddressSpace::is_mapped(VirtAddr  virt,
                              usize     size,
                              PageFlags required_flags) const noexcept {
    if (!m_context.is_valid() || size == 0) return false;
    if (!is_page_aligned(virt))             return false;

    // Fast path: single-VMA range
    VMArea* vma = m_vma_tree.find(virt);
    if (vma && vma->start <= virt && vma->end() >= virt + size) {
        // The range is entirely within one VMA.
        // Check that the VMA's flags are a superset of required_flags.
        PageFlags intersection = vma->flags & required_flags;
        return intersection == required_flags;
    }

    // Slow path: walk page-by-page via the hardware backend
    const usize page_count = (size + PAGE_MASK) / PAGE_SIZE;
    for (usize i = 0; i < page_count; ++i) {
        VirtAddr page_va = virt + i * PAGE_SIZE;
        PhysAddr pa = m_backend->translate(m_context, page_va);
        if (pa == 0) return false; // Page not present.

        // To verify flags we need the VMA for this page.
        VMArea* v = m_vma_tree.find(page_va);
        if (!v) return false;

        PageFlags intersection = v->flags & required_flags;
        if (intersection != required_flags) return false;
    }

    return true;
}


VMArea* AddressSpace::find_vma(VirtAddr addr) const noexcept {
    return m_vma_tree.find(addr);
}

VMArea* AddressSpace::find_overlap(VirtAddr addr, usize size) const noexcept {
    return m_vma_tree.find_overlap(addr, size);
}


VirtAddr AddressSpace::find_free_region(VirtAddr hint, usize size) const noexcept {
    if (size == 0) return 0;

    VirtAddr search_start;
    VirtAddr search_limit;

    if (m_is_kernel) {
        search_start = KERNEL_START;
        search_limit = UINTPTR_MAX - PAGE_SIZE; // Leave room for guard
    } else {
        search_start = USER_START;
        search_limit = USER_END;
    }

    // Clamp hint into the valid range.
    if (hint < search_start) hint = search_start;
    if (hint >= search_limit) return 0;

    return m_vma_tree.find_free_region(hint, size, search_limit);
}

void AddressSpace::activate() noexcept {
    if (m_context.is_valid())
        m_backend->activate(m_context);
}
