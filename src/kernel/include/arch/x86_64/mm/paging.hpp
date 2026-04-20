// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VMM_X86_64_HPP
#define VMM_X86_64_HPP

#include <stdint.h>

#define PAGE_SIZE 4096

// Index extraction macros
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)

namespace arch::mem {
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


    inline void invlpg(uint64_t virt) {
        asm volatile("invlpg (%0)" ::"r" (virt) : "memory");
    }

    inline void write_cr3(uint64_t pml4_phys) {
        asm volatile("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
    }

    inline uint64_t read_cr3() {
        uint64_t cr3;
        asm volatile("mov %%cr3, %0" : "=r"(cr3));
        return cr3;
    }
} // namespace arch::mem

#endif // VMM_X86_64_HPP