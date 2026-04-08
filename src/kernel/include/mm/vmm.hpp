// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VMM_HPP
#define VMM_HPP

#include <stdint.h>

namespace vmm
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

} // namespace vmm

#endif // VMM_HPP