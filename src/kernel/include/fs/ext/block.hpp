// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef EXT2_BLOCK_HPP
#define EXT2_BLOCK_HPP

#include <stdint.h>
#include <fs/ext/ext2.hpp>

namespace ext2 {
    // Block specific functions

    // Read / Write functions
    bool read_block(ext2_fs_t* fs, const uint32_t block_num, uint8_t* buffer, const uint32_t blocks_to_read = 1);
    bool write_block(ext2_fs_t* fs, const uint32_t block_num, uint8_t* buffer, const uint32_t blocks_to_write = 1);

    // Alloc/free, bitmap
    uint8_t* get_block_bitmap(ext2_fs_t* fs, uint32_t group);
    void write_block_bitmap(ext2_fs_t* fs, uint32_t group, uint8_t* bitmap);
    // Allocates block
    uint32_t alloc_block(ext2_fs_t* fs);
    void free_block(ext2_fs_t* fs, uint32_t block_num);
    void free_blocks(ext2_fs_t* fs, inode_t* inode);
} // namespace ext2

#endif // EXT2_BLOCK_HPP
