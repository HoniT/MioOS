// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VMM_X86_64_HPP
#define VMM_X86_64_HPP

#include <mm/address_space.hpp>
#include <mm/vma_slab.hpp>
#include <arch/x86_64/mm/paging.hpp>

namespace mem {
    constexpr VirtAddr PHYS_MAP_OFFSET = 0xFFFF'FFFF'8000'0000ULL;
    
    struct VMMState {
        mem::VMASlabAllocator            m_slab;
        arch::mem::X86_64PagingBackend m_backend;
        AddressSpace                m_kernel_as;
        
        VMMState(PhysAddr boot_pml4_phys,
                VirtAddr hhdm_offset = PHYS_MAP_OFFSET) noexcept
            : m_slab      (hhdm_offset)
            , m_backend   (boot_pml4_phys, hhdm_offset)   // adopt boot PML4
            , m_kernel_as (&m_backend,
                        vma_slab_alloc,
                        vma_slab_free,
                        &m_slab,
                        true)                           // is_kernel = true
        {}
        
        VMMState(const VMMState&)            = delete;
        VMMState& operator=(const VMMState&) = delete;
    };
    
    /// One-Shot Validation + Activation
    void vmm_init() noexcept;
} // namespace mem
    
#endif // VMM_X86_64_HPP
    