// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef MULTIBOOT_HPP
#define MULTIBOOT_HPP

#include <stdint.h>

// ============================================
// Multiboot2 magic numbers
// ============================================
// This is in the HEADER (in boot.asm):
#define MULTIBOOT2_HEADER_MAGIC        0xE85250D6

// This is what GRUB passes to kernel in EAX:
#define MULTIBOOT2_BOOTLOADER_MAGIC    0x36D76289

// ============================================
// Multiboot2 Tag Types
// ============================================
#define MULTIBOOT_TAG_TYPE_END               0
#define MULTIBOOT_TAG_TYPE_CMDLINE           1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME  2
#define MULTIBOOT_TAG_TYPE_MODULE            3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO     4
#define MULTIBOOT_TAG_TYPE_BOOTDEV           5
#define MULTIBOOT_TAG_TYPE_MMAP              6
#define MULTIBOOT_TAG_TYPE_VBE               7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER       8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS      9
#define MULTIBOOT_TAG_TYPE_APM               10
#define MULTIBOOT_TAG_TYPE_EFI32             11
#define MULTIBOOT_TAG_TYPE_EFI64             12
#define MULTIBOOT_TAG_TYPE_SMBIOS            13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD          14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW          15
#define MULTIBOOT_TAG_TYPE_NETWORK           16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP          17
#define MULTIBOOT_TAG_TYPE_EFI_BS            18
#define MULTIBOOT_TAG_TYPE_EFI32_IH          19
#define MULTIBOOT_TAG_TYPE_EFI64_IH          20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR    21

// ============================================
// Multiboot2 Structures
// ============================================

// The main Multiboot2 information structure
struct multiboot2_info {
    uint32_t total_size;
    uint32_t reserved;
    // Tags follow immediately after this
} __attribute__((packed));

// Base tag structure
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

// Command line tag
struct multiboot_tag_string {
    uint32_t type;
    uint32_t size;
    char string[0];  // Flexible array member
} __attribute__((packed));

// Basic memory info tag
struct multiboot_tag_basic_meminfo {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;
    uint32_t mem_upper;
} __attribute__((packed));

// Memory map entry
struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed));

// Memory map tag
struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot_mmap_entry entries[0];
} __attribute__((packed));

struct multiboot_tag_bootdev {
    uint32_t type;
    uint32_t size;
    uint32_t biosdev;      // BIOS drive number
    uint32_t partition;     // Partition number (0xFFFFFFFF if whole disk)
    uint32_t sub_partition; // Sub-partition (0xFFFFFFFF if none)
} __attribute__((packed));

// Framebuffer tag
struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint16_t reserved;
    
    // Color info follows (variable size based on type)
    union {
        struct {
            uint32_t num_colors;
            // Palette entries follow
        } palette;
        struct {
            uint8_t red_field_position;
            uint8_t red_mask_size;
            uint8_t green_field_position;
            uint8_t green_mask_size;
            uint8_t blue_field_position;
            uint8_t blue_mask_size;
        } rgb;
    } color_info;
} __attribute__((packed));

// Framebuffer types
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED   0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB       1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT  2

// Memory types
#define MULTIBOOT_MEMORY_AVAILABLE        1
#define MULTIBOOT_MEMORY_RESERVED         2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS              4
#define MULTIBOOT_MEMORY_BADRAM           5

// ============================================
// Helper Functions
// ============================================

class Multiboot2 {
public:
    static void* find_tag(void* mb2_info, uint32_t tag_type);
    
    static multiboot_tag_framebuffer* get_framebuffer(void* mb2_info);
    
    static multiboot_tag_basic_meminfo* get_basic_meminfo(void* mb2_info);
    
    static multiboot_tag_string* get_cmdline(void* mb2_info);
    
    static multiboot_tag_string* get_bootloader_name(void* mb2_info);
    
    static multiboot_tag_mmap* get_mmap(void* mb2_info);

    static multiboot_tag_bootdev* get_bootdev(void* mb2_info);
};

#endif // MULTIBOOT_HPP
