// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <mm/vmm.hpp>
#include <mm/pmm.hpp>

namespace mem
{
    AddressSpace* VMM::kernel_space = nullptr;

    inline void* VMM::phys_to_virt(void* phys) {
        return (void*)((uint64_t)phys + mem::HIGHER_HALF_OFFSET);
    }

    inline void* VMM::virt_to_phys(void* virt) {
        return (void*)((uint64_t)virt - mem::HIGHER_HALF_OFFSET);
    }

    inline void VMM::invlpg(void* vaddr) {
        asm volatile("invlpg (%0)" ::"r" (vaddr) : "memory");
    }

    /// @brief Custom memory set for freestanding environments
    static void zero_page(void* virt_addr) {
        uint64_t* ptr = (uint64_t*)virt_addr;
        for (size_t i = 0; i < 512; i++) ptr[i] = 0;
    }

    AddressSpace* VMM::get_kernel_space() {
        return kernel_space;
    }
} // namespace mem
