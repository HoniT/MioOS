// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VMM_HPP
#define VMM_HPP

#include <stdint.h>
#include <stddef.h>
#include <mm/vma.hpp>

namespace mem
{

    /**
     * @brief Represents a complete Virtual Address Space (a single CPU context).
     * * The kernel possesses one master AddressSpace. Every user-mode process 
     * instantiates its own AddressSpace, which automatically links the higher-half 
     * kernel mappings upon creation.
     */
    class AddressSpace {
    public:
        AddressSpace();
        ~AddressSpace();

        [[nodiscard]] void* allocate_region(uint64_t hint_addr, size_t size, PageFlags flags, VMAType type);
        bool free_region(uint64_t addr, size_t size);

        [[nodiscard]] VMArea* get_vma(uint64_t address);

        bool map_page_immediate(uint64_t virt, uint64_t phys, PageFlags flags);
        void unmap_page_immediate(uint64_t virt);

        [[nodiscard]] uint64_t get_physical_address(uint64_t virt_addr) const;

        void activate();

        [[nodiscard]] inline void* get_root() const { return root_phys; }

    private:
        void* root_phys;
        void* root_virt;
        VMATree vma_tree;

        // Spinlock lock; // (TODO in far future)
    };

    extern AddressSpace* kernel_address_space;
    void init_vmm();

} // namespace mem

namespace hal::mem
{
    bool map_page(void* root_virt, uint64_t virt, uint64_t phys, uint64_t flags);
    void unmap_page(void* root_virt, uint64_t virt);
    uint64_t get_phys_address(void* root_virt, uint64_t virt);
} // namespace hal::mem


#endif // VMM_HPP