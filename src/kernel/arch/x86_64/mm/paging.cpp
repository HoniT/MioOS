// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <arch/x86_64/mm/paging.hpp>
#include <mm/vmm.hpp>

namespace hal::mem {
    /// @brief Maps a page at the hardware level
    /// @param pml4_virt PML4 virtual address
    /// @param virt Virtual address on where to map the page
    /// @param phys Physical address of the page
    /// @param flags Flags
    /// @return If the operation was a success
    bool map_page(void* pml4_virt, uint64_t virt, uint64_t phys, uint64_t flags) {

    }

    /// @brief Frees a page at the hardware level
    /// @param pml4_virt PML4 virtual address
    /// @param virt Page virtual address
    void unmap_page(void* pml4_virt, uint64_t virt) {

    }

    /// @brief Gets physical address from a virtual address
    /// @param pml4_virt PML4 virtual address
    uint64_t get_phys_address(void* pml4_virt, uint64_t virt) {

    }
} // namespace arch::mem