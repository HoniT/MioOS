// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VMM_HPP
#define VMM_HPP

#include <stdint.h>

namespace mem
{
    #pragma region Structs

    // PML4 Entry
    typedef union {
        uint64_t raw;
        struct {
            uint64_t present         : 1;
            uint64_t read_write      : 1;
            uint64_t user_supervisor : 1;
            uint64_t write_through   : 1;
            uint64_t cache_disable   : 1;
            uint64_t accessed        : 1;
            uint64_t ignored_1       : 1;
            uint64_t page_size       : 1;
            uint64_t ignored_2       : 3;
            uint64_t restart         : 1;
            uint64_t phys_addr       : 40;
            uint64_t ignored_3       : 11;
            uint64_t execute_disable : 1;
        } __attribute__((packed));
    } pml4e_t;

    // PDPT Entry (References a Page Directory or Maps a 1GB Page)
    typedef union {
        uint64_t raw;
        
        struct {
            uint64_t present         : 1;
            uint64_t read_write      : 1;
            uint64_t user_supervisor : 1;
            uint64_t write_through   : 1;
            uint64_t cache_disable   : 1;
            uint64_t accessed        : 1;
            uint64_t ignored_1       : 1;
            uint64_t page_size       : 1;
            uint64_t ignored_2       : 3;
            uint64_t restart         : 1;
            uint64_t phys_addr       : 40;
            uint64_t ignored_3       : 11;
            uint64_t execute_disable : 1;
        } __attribute__((packed)) dir;

        struct {
            uint64_t present         : 1;
            uint64_t read_write      : 1;
            uint64_t user_supervisor : 1;
            uint64_t write_through   : 1;
            uint64_t cache_disable   : 1;
            uint64_t accessed        : 1;
            uint64_t dirty           : 1;
            uint64_t page_size       : 1;
            uint64_t global          : 1;
            uint64_t ignored_1       : 2;
            uint64_t restart         : 1;
            uint64_t pat             : 1;
            uint64_t reserved_1      : 17;
            uint64_t phys_addr       : 22;
            uint64_t ignored_2       : 7;
            uint64_t prot_key        : 4;
            uint64_t execute_disable : 1;
        } __attribute__((packed)) page_1gb;
    } pdpte_t;

    // Page Directory Entry (References a Page Table or Maps a 2MB Page)
    typedef union {
        uint64_t raw;

        struct {
            uint64_t present         : 1;
            uint64_t read_write      : 1;
            uint64_t user_supervisor : 1;
            uint64_t write_through   : 1;
            uint64_t cache_disable   : 1;
            uint64_t accessed        : 1;
            uint64_t ignored_1       : 1;
            uint64_t page_size       : 1;
            uint64_t ignored_2       : 3;
            uint64_t restart         : 1;
            uint64_t phys_addr       : 40;
            uint64_t ignored_3       : 11;
            uint64_t execute_disable : 1;
        } __attribute__((packed)) table;

        struct {
            uint64_t present         : 1;
            uint64_t read_write      : 1;
            uint64_t user_supervisor : 1;
            uint64_t write_through   : 1;
            uint64_t cache_disable   : 1;
            uint64_t accessed        : 1;
            uint64_t dirty           : 1;
            uint64_t page_size       : 1;
            uint64_t global          : 1;
            uint64_t ignored_1       : 2;
            uint64_t restart         : 1;
            uint64_t pat             : 1;
            uint64_t reserved_1      : 8;
            uint64_t phys_addr       : 31;
            uint64_t ignored_2       : 7;
            uint64_t prot_key        : 4;
            uint64_t execute_disable : 1;
        } __attribute__((packed)) page_2mb;
    } pde_t;

    // Page Table Entry (Maps a 4KB Page)
    typedef union {
        uint64_t raw;
        struct {
            uint64_t present         : 1;
            uint64_t read_write      : 1;
            uint64_t user_supervisor : 1;
            uint64_t write_through   : 1;
            uint64_t cache_disable   : 1;
            uint64_t accessed        : 1;
            uint64_t dirty           : 1;
            uint64_t pat             : 1;
            uint64_t global          : 1;
            uint64_t ignored_1       : 2;
            uint64_t restart         : 1;
            uint64_t phys_addr       : 40;
            uint64_t ignored_2       : 7;
            uint64_t prot_key        : 4;
            uint64_t execute_disable : 1;
        } __attribute__((packed));
    } pte_t;

    #pragma endregion

    #pragma region Flags & Enums

    enum PageFlags : uint64_t {
        Present         = 1ULL << 0,
        ReadWrite       = 1ULL << 1,
        User            = 1ULL << 2,
        WriteThrough    = 1ULL << 3,
        CacheDisable    = 1ULL << 4,
        Accessed        = 1ULL << 5,
        Dirty           = 1ULL << 6,
        HugePage        = 1ULL << 7, // 2MB/1GB
        Global          = 1ULL << 8,
        NoExecute       = 1ULL << 63
    };

    enum VMAType {
        VMA_HEAP,
        VMA_STACK,
        VMA_CODE,
        VMA_MMAP,
        VMA_ANONYMOUS
    };

    // Virtual Memory Area - Tracks logical memory regions
    struct VMA {
        uint64_t start_addr;
        uint64_t end_addr;
        uint64_t flags;
        VMAType type;
        
        VMA* next; // Singly linked list for this AddressSpace
    };

    #pragma endregion

    class Spinlock {
        /* TODO in future */
    };

    // Represents a single Process's Memory Space
    class AddressSpace {
    public:
        AddressSpace();
        ~AddressSpace();

        // Hardware Mapping
        bool map_page(void* phys_addr, void* virt_addr, uint64_t flags);
        void unmap_page(void* virt_addr);
        void* get_phys_addr(void* virt_addr);
        
        // Logical Mapping (Demand Paging)
        bool allocate_vma(uint64_t virt_addr, uint64_t size, uint64_t flags, VMAType type);
        bool handle_page_fault(uint64_t fault_addr, uint32_t error_code);

        void activate(); // Load this space into CR3

        pml4e_t* get_pml4() { return pml4; }

    private:
        pml4e_t* pml4;
        void* pml4_phys;
        
        VMA* vma_list;
        Spinlock lock;

        void free_table_hierarchy(pml4e_t* pml4_virt);
    };

    class VMM {
    public:
        static void init();
        static AddressSpace* get_kernel_space();

        static inline void* phys_to_virt(void* phys);
        static inline void* virt_to_phys(void* virt);
        static inline void invlpg(void* vaddr);

    private:
        static AddressSpace* kernel_space;
    };

} // namespace mem

#endif // VMM_HPP