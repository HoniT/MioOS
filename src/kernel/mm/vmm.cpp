// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <mm/vmm.hpp>

namespace mem
{
    AddressSpace::AddressSpace() {
        
    }

    AddressSpace::~AddressSpace() {
        
    }

    /// @brief Allocates a region
    [[nodiscard]] void* AddressSpace::allocate_region(uint64_t hint_addr, size_t size, PageFlags flags, VMAType type) {

    }

    /// @brief Frees a region
    bool AddressSpace::free_region(uint64_t addr, size_t size) {

    }

    /// @brief Gets a VMA by a virtual address
    [[nodiscard]] VMArea* AddressSpace::get_vma(uint64_t address) {

    }

    /// @brief Maps a page directly without VMA interference 
    bool AddressSpace::map_page_immediate(uint64_t virt, uint64_t phys, PageFlags flags) {

    }

    /// @brief Unmaps a page directly without VMA interference 
    void AddressSpace::unmap_page_immediate(uint64_t virt) {

    }

    /// @brief Gets physical address from virtual address
    [[nodiscard]] uint64_t AddressSpace::get_physical_address(uint64_t virt_addr) const {

    }

    /// @brief Activates the address space by placing it in the CPU
    void AddressSpace::activate() {

    }

    AddressSpace* kernel_address_space;

    /// @brief Initializes the VMM
    void init_vmm() {

    }

} // namespace mem
