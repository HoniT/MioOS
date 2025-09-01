// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef KERNEL_MAIN_HPP
#define KERNEL_MAIN_HPP

#include <stdint.h>
#include <lib/data/string.hpp>

// GRUB multiboot magic number
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

// MioOS kernel version
extern data::string kernel_version;

// GRUB multiboot info
// Multiboot header definitions
struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    // Other fields...
};

extern "C" void kernel_main(uint32_t magic, multiboot_info* mbi);

#endif // KERNEL_MAIN_HPP
