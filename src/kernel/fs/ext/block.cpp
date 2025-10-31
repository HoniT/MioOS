// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// block.cpp
// Sets up Ext2 block functions
// ========================================

#include <fs/ext/block.hpp>
#include <fs/ext/inode.hpp>
#include <graphics/vga_print.hpp>
#include <drivers/ata.hpp>
#include <interrupts/kernel_panic.hpp>
#include <mm/heap.hpp>

bool ext2::read_block(ext2_fs_t* fs, const uint32_t block_num, uint8_t* buffer, const uint32_t blocks_to_read) {
    // Translating Ext2 blocks to LBA blocks
    uint32_t lba = fs->partition_start + ((block_num * fs->block_size) / 512);
    uint32_t lba_blocks = (blocks_to_read * fs->block_size) / 512;

    if (fs->dev) {
        // ATA driver expects words, so cast here only
        pio_28::read_sector(fs->dev, lba, reinterpret_cast<uint16_t*>(buffer), lba_blocks);
    }
    else {
        kprintf(LOG_ERROR, "Ext2 FS doesn't have a device!\n");
        return false;
    }
    return true;
}

bool ext2::write_block(ext2_fs_t* fs, const uint32_t block_num, uint8_t* buffer, const uint32_t blocks_to_write) {
    uint32_t lba = fs->partition_start + ((block_num * fs->block_size) / 512);
    uint32_t lba_blocks = (blocks_to_write * fs->block_size) / 512;

    if (fs->dev) {
        pio_28::write_sector(fs->dev, lba, reinterpret_cast<uint16_t*>(buffer), lba_blocks);
    }
    else {
        kprintf(LOG_ERROR, "Ext2 FS doesn't have a device!\n");
        return false;
    }
    return true;
}

/// @brief Returns pointer to the block bitmap of a given block group
/// @param fs Filesystem pointer
/// @param group Block group index
/// @return Pointer to bitmap in memory
uint8_t* ext2::get_block_bitmap(ext2_fs_t* fs, uint32_t group) {
    if (!fs) return nullptr;

    // Compute total number of groups
    uint32_t total_groups = (fs->sb->blks_num + fs->sb->blkgroup_blk_num - 1) / fs->sb->blkgroup_blk_num;
    if (group >= total_groups) return nullptr;

    uint32_t bitmap_block = fs->blk_grp_descs[group].blk_addr_blk_usage_bitmap;
    uint8_t* buf = (uint8_t*)kmalloc(fs->block_size);
    ext2::read_block(fs, bitmap_block, buf);
    return buf;
}

/// @brief Writes block bitmap back to disk
/// @param fs Filesystem pointer
/// @param group Block group index
/// @param bitmap Pointer to bitmap in memory
void ext2::write_block_bitmap(ext2_fs_t* fs, uint32_t group, uint8_t* bitmap) {
    if (!fs || !bitmap) return;

    // Compute total number of groups
    uint32_t total_groups = (fs->sb->blks_num + fs->sb->blkgroup_blk_num - 1) / fs->sb->blkgroup_blk_num;
    if (group >= total_groups) return;

    uint32_t bitmap_block = fs->blk_grp_descs[group].blk_addr_blk_usage_bitmap;
    ext2::write_block(fs, bitmap_block, bitmap); // write_block writes memory back to disk
}


/// @brief Allocates a new block
/// @param fs File system on where to allocate
/// @return Allocated block index
uint32_t ext2::alloc_block(ext2_fs_t* fs) {
    if (!fs) kernel_panic("alloc_block: invalid filesystem pointer!");

    // Compute total number of block groups
    uint32_t total_groups = (fs->sb->blks_num + fs->sb->blkgroup_blk_num - 1) / fs->sb->blkgroup_blk_num;

    for (uint32_t group = 0; group < total_groups; group++) {
        // Skip group if no unallocated blocks
        if (!fs->blk_grp_descs[group].num_unalloc_blks) continue;

        // Get block bitmap for this group
        uint8_t* bitmap = ext2::get_block_bitmap(fs, group);
        if (!bitmap) kernel_panic("alloc_block: failed to get block bitmap!");

        // Iterate through each block in this group
        for (uint32_t blk_offset = 0; blk_offset < fs->blocks_per_group; blk_offset++) {
            uint32_t byte_index = blk_offset / 8;
            uint8_t bit_index = blk_offset % 8;

            // Skip if block already allocated
            if (bitmap[byte_index] & (1 << bit_index)) continue;

            // Mark block as allocated
            bitmap[byte_index] |= (1 << bit_index);
            ext2::write_block_bitmap(fs, group, bitmap);

            // Update metadata
            fs->blk_grp_descs[group].num_unalloc_blks--;
            fs->sb->unalloc_blk_num--;
            ext2::rewrite_bgds(fs);
            ext2::rewrite_sb(fs);
            kfree(bitmap);

            // Calculate and return absolute block number
            return group * fs->blocks_per_group + blk_offset + fs->sb->blkgrp_superblk;
        }
    }

    // No free blocks found
    kernel_panic("No more blocks to allocate!");
    return (uint32_t)-1;
}


/// @brief Free a single block
/// @param fs Filesystem pointer
/// @param block_num Absolute block number
void ext2::free_block(ext2_fs_t* fs, uint32_t block_num) {
    if (!fs || block_num == 0) return;

    // Compute group and offset
    uint32_t blocks_per_group = fs->sb->blkgroup_blk_num;
    uint32_t group = (block_num - fs->sb->blkgrp_superblk) / blocks_per_group;
    uint32_t offset = (block_num - fs->sb->blkgrp_superblk) % blocks_per_group;

    // Load block bitmap
    uint8_t* bitmap = get_block_bitmap(fs, group);
    if (!bitmap) return;

    // Clear bit
    clear_bitmap_bit(bitmap, offset);

    // Update metadata
    fs->blk_grp_descs[group].num_unalloc_blks++;
    fs->sb->unalloc_blk_num++;

    // Write back bitmap + descriptors
    write_block_bitmap(fs, group, bitmap);
    kfree(bitmap);
    rewrite_bgds(fs);
    rewrite_sb(fs);
}

/// @brief Free all data blocks used by an inode
/// @param fs Filesystem pointer
/// @param inode The inode whose blocks to free
void ext2::free_blocks(ext2_fs_t* fs, inode_t* inode) {
    if (!fs || !inode) return;

    // Free direct blocks
    for (int i = 0; i < 12; i++) {
        if (inode->direct_blk_ptr[i]) {
            free_block(fs, inode->direct_blk_ptr[i]);
            inode->direct_blk_ptr[i] = 0;
        }
    }

    // Free singly-indirect
    if (inode->singly_inderect_blk_ptr) {
        uint32_t* blocks = (uint32_t*)kmalloc(fs->block_size);
        read_block(fs, inode->singly_inderect_blk_ptr, (uint8_t*)blocks);
        for (uint32_t i = 0; i < fs->sb->blk_size / sizeof(uint32_t); i++) {
            if (blocks[i]) free_block(fs, blocks[i]);
        }
        free_block(fs, inode->singly_inderect_blk_ptr);
        inode->singly_inderect_blk_ptr = 0;
        kfree(blocks);
    }

    // Free doubly-indirect
    if (inode->doubly_inderect_blk_ptr) {
        uint32_t* level1 = (uint32_t*)kmalloc(fs->block_size);
        read_block(fs, inode->doubly_inderect_blk_ptr, (uint8_t*)level1);
        for (uint32_t i = 0; i < fs->sb->blk_size / sizeof(uint32_t); i++) {
            if (level1[i]) {
                uint32_t* level2 = (uint32_t*)kmalloc(fs->block_size);
                read_block(fs, level1[i], (uint8_t*)level2);
                for (uint32_t j = 0; j < fs->sb->blk_size / sizeof(uint32_t); j++) {
                    if (level2[j]) free_block(fs, level2[j]);
                }
                free_block(fs, level1[i]);
                kfree(level2);
            }
        }
        free_block(fs, inode->doubly_inderect_blk_ptr);
        inode->doubly_inderect_blk_ptr = 0;
        kfree(level1);
    }

    // Free triply-indirect
    if (inode->triply_inderect_blk_ptr) {
        uint32_t* level1 = (uint32_t*)kmalloc(fs->block_size);
        read_block(fs, inode->triply_inderect_blk_ptr, (uint8_t*)level1);
        for (uint32_t i = 0; i < fs->sb->blk_size / sizeof(uint32_t); i++) {
            if (level1[i]) {
                uint32_t* level2 = (uint32_t*)kmalloc(fs->block_size);
                read_block(fs, level1[i], (uint8_t*)level2);
                for (uint32_t j = 0; j < fs->sb->blk_size / sizeof(uint32_t); j++) {
                    if (level2[j]) {
                        uint32_t* level3 = (uint32_t*)kmalloc(fs->block_size);
                        read_block(fs, level2[j], (uint8_t*)level3);
                        for (uint32_t k = 0; k < fs->sb->blk_size / sizeof(uint32_t); k++) {
                            if (level3[k]) free_block(fs, level3[k]);
                        }
                        free_block(fs, level2[j]);
                        kfree(level3);
                    }
                }
                free_block(fs, level1[i]);
                kfree(level2);
            }
        }
        free_block(fs, inode->triply_inderect_blk_ptr);
        inode->triply_inderect_blk_ptr = 0;
        kfree(level1);
    }
}
