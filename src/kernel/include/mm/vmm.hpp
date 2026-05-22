// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef VMM_HPP
#define VMM_HPP

#include <mm/vma.hpp>
#include <mm/paging.hpp>
#include <mm/address_space.hpp>
#include <mm/mm_defs.hpp>

namespace mem
{
    class VMM {
    public:
        VMASlabAllocator            m_slab;
        PagingBackend               m_backend;
        AddressSpace                m_kernel_as;
        
        VMM(PhysAddr boot_pml4_phys,
                VirtAddr hhdm_offset = HIGHER_HALF_OFFSET) noexcept
            : m_slab      (hhdm_offset)
            , m_backend   (boot_pml4_phys, hhdm_offset)   // adopt boot PML4
            , m_kernel_as (&m_backend,
                        ::mem::vma_slab_alloc,
                        ::mem::vma_slab_free,
                        &m_slab,
                        true)                           // is_kernel = true
        {}
        
        VMM(const VMM&)            = delete;
        VMM& operator=(const VMM&) = delete;

        /// One-Shot Validation + Activation
        static void init() noexcept;
    };
} // namespace mem


#endif // VMM_HPP
