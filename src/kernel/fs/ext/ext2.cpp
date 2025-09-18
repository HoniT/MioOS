// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// ext2.cpp
// Sets up the Ext2 file system
// ========================================

#include <fs/ext/ext2.hpp>
#include <fs/ext/vfs.hpp>
#include <mbr.hpp>
#include <mm/heap.hpp>
#include <drivers/ata.hpp>
#include <drivers/vga_print.hpp>
#include <interrupts/kernel_panic.hpp>
#include <rtc.hpp>
#include <kterminal.hpp>
#include <lib/mem_util.hpp>
#include <lib/data/string.hpp>
#include <lib/string_util.hpp>
#include <lib/path_util.hpp>

#pragma region Read & Write

/// @brief Returns pointer to the inode bitmap of a given block group
/// @param fs Filesystem pointer
/// @param group Block group index
/// @return Pointer to bitmap in memory
uint8_t* ext2::get_inode_bitmap(ext2_fs_t* fs, uint32_t group) {
    if(!fs) return nullptr;
    uint32_t groups_count = (fs->sb->blks_num + fs->sb->blkgroup_blk_num - 1) / fs->sb->blkgroup_blk_num;
    if(group >= groups_count) return nullptr;

    uint32_t bitmap_block = fs->blk_grp_descs[group].blk_addr_inode_usage_bitmap;
    uint8_t* buf = (uint8_t*)kmalloc(fs->block_size);
    ext2::read_block(fs, bitmap_block, buf);

    return buf;
}

/// @brief Writes inode bitmap back to disk
/// @param fs Filesystem pointer
/// @param group Block group index
/// @param bitmap Pointer to bitmap in memory
void ext2::write_inode_bitmap(ext2_fs_t* fs, uint32_t group, uint8_t* bitmap) {
    if(!fs || !bitmap) return;
    uint32_t groups_count = (fs->sb->blks_num + fs->sb->blkgroup_blk_num - 1) / fs->sb->blkgroup_blk_num;
    if(group >= groups_count) return;

    uint32_t bitmap_block = fs->blk_grp_descs[group].blk_addr_inode_usage_bitmap;
    ext2::write_block(fs, bitmap_block, bitmap); // write_block writes memory back to disk
}


// Loads an inode with a given inode number
inode_t* ext2::load_inode(ext2_fs_t* fs, const uint32_t inode_num) {
    // Determining which Block Group contains an Inode
    uint32_t group = (inode_num - 1) / fs->sb->blkgroup_inode_num;
    // Finding an inode inside of the Block Group
    uint32_t index = (inode_num - 1) % fs->sb->blkgroup_inode_num;
    // Retreiving BGD
    blkgrp_descriptor_t* desc = &fs->blk_grp_descs[group];

    uint32_t inode_size = fs->sb->inode_size;
    uint32_t inode_table_block = desc->inode_tbl_start_blk_addr;
    uint32_t offset = index * inode_size;

    uint32_t block_num = inode_table_block + (offset / fs->block_size);
    uint32_t offset_in_block = offset % fs->block_size;

    uint8_t* buffer = (uint8_t*)kmalloc(fs->block_size);
    ext2::read_block(fs, block_num, buffer);
    return (inode_t*)(buffer + offset_in_block);
}

/// @brief Free an inode in ext2
/// @param fs Filesystem pointer
/// @param inode_num The inode number to free
void ext2::free_inode(ext2_fs_t* fs, const uint32_t inode_num) {
    if(!fs || inode_num == 0) return;

    // Inodes are 1-based in ext2
    uint32_t index = inode_num - 1;

    // Locate group and position within group
    uint32_t inodes_per_group = fs->sb->blkgroup_inode_num;
    uint32_t group = index / inodes_per_group;
    uint32_t offset = index % inodes_per_group;

    // Load inode bitmap of this group
    uint8_t* inode_bitmap = get_inode_bitmap(fs, group);

    // Clear bit
    clear_bitmap_bit(inode_bitmap, offset);

    // Update group descriptor
    fs->blk_grp_descs[group].num_unalloc_inodes++;

    // Update superblock
    fs->sb->unalloc_inode_num++;

    // Write back bitmap + descriptors
    write_inode_bitmap(fs, group, inode_bitmap);
    rewrite_bgds(fs);
    rewrite_sb(fs);
}


/// @brief Allocates a new inode
/// @param fs File system on where to allocate
/// @return Allocated inode index
uint32_t ext2::alloc_inode(ext2_fs_t* fs) {
    // Compute total number of block groups
    uint32_t total_groups = (fs->sb->blks_num + fs->sb->blkgroup_blk_num - 1) / fs->sb->blkgroup_blk_num;

    for (uint32_t group = 0; group < total_groups; group++) {
        // Skip group if no unallocated inodes
        if (!fs->blk_grp_descs[group].num_unalloc_inodes) continue;

        // Get inode bitmap for this group
        uint8_t* bitmap = ext2::get_inode_bitmap(fs, group);
        if (!bitmap) kernel_panic("alloc_inode: failed to get inode bitmap!");

        // Iterate through each inode in this group
        for (uint32_t ino_offset = 0; ino_offset < fs->inodes_per_group; ino_offset++) {
            // Compute 1-based inode number
            uint32_t ino = group * fs->inodes_per_group + ino_offset + 1;

            // Skip reserved inodes
            if (ino < EXT2_FIRST_NONRESERVED_INO) continue;

            // The bitmap bit for inode `ino` is at index `ino - 1`
            if (!TEST_BIT(bitmap, ino - 1)) {
                // Mark as allocated (bit index = ino - 1)
                SET_BIT(bitmap, ino - 1);

                // Write bitmap back
                ext2::write_inode_bitmap(fs, group, bitmap);

                // Update metadata
                fs->blk_grp_descs[group].num_unalloc_inodes--;
                fs->sb->unalloc_inode_num--;
                ext2::rewrite_bgds(fs);
                ext2::rewrite_sb(fs);

                // free bitmap buffer we got from get_inode_bitmap
                kfree(bitmap);

                return ino; // return 1-based inode number
            }
        }

        // nothing found in this group, free bitmap before moving on
        kfree(bitmap);
    }

    kernel_panic("No more inodes to allocate!");
    return (uint32_t)-1;
}

// Writes inode info to disk
void ext2::write_inode(ext2_fs_t* fs, const uint32_t inode_num, inode_t* inode) {
    // Inode index is 1-based in Ext2
    uint32_t index = inode_num - 1;
    uint32_t group = index / fs->inodes_per_group;
    uint32_t index_in_group = index % fs->inodes_per_group;

    uint32_t inode_table_block = fs->blk_grp_descs[group].inode_tbl_start_blk_addr;

    // Use on-disk inode size, not sizeof(inode_t)
    uint32_t inode_size = fs->sb->inode_size;
    uint32_t offset = index_in_group * inode_size;

    uint32_t block_offset = offset / fs->block_size;
    uint32_t byte_offset  = offset % fs->block_size;

    uint32_t block_num = inode_table_block + block_offset;

    // Reading the block where the inode is
    uint8_t* buf = (uint8_t*)kcalloc(1, fs->block_size);
    if (!buf) {
        return; // allocation failure
    }

    ext2::read_block(fs, block_num, buf);

    // Copying given info to buffer
    // Copy only inode_size (truncate or pad relative to sizeof(inode_t))
    uint32_t copy_size = (sizeof(inode_t) < inode_size) ? sizeof(inode_t) : inode_size;
    memcpy(buf + byte_offset, inode, copy_size);

    // Rewriting block
    ext2::write_block(fs, block_num, buf);

    kfree(buf);
    return; // success
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
            }
        }
        free_block(fs, inode->doubly_inderect_blk_ptr);
        inode->doubly_inderect_blk_ptr = 0;
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
                    }
                }
                free_block(fs, level1[i]);
            }
        }
        free_block(fs, inode->triply_inderect_blk_ptr);
        inode->triply_inderect_blk_ptr = 0;
    }
}

// Rewrites block group descriptors of a FS
void ext2::rewrite_bgds(ext2_fs_t* fs) {
    uint32_t bgd_start_block = (fs->block_size == 1024) ? 2 : 1;

    uint8_t* base = (uint8_t*)fs->blk_grp_descs;
    for(uint32_t i = 0; i < fs->blk_grp_desc_blocks; i++) 
        ext2::write_block(fs, bgd_start_block + i, base + i * fs->block_size);
}

// Rewrites superblock of a FS
void ext2::rewrite_sb(ext2_fs_t* fs) {
    if(!fs) return;

    uint8_t* buf = (uint8_t*)kcalloc(1, SUPERBLOCK_SIZE);

    // Read existing block first
    pio_28::read_sector(fs->dev, 2, reinterpret_cast<uint16_t*>(buf), 2);

    // Copy updated superblock into the correct offset
    // Goes at offset 0 in block #1
    memcpy(buf, &fs->sb, SUPERBLOCK_SIZE);

    // Write back
    pio_28::read_sector(fs->dev, 2, reinterpret_cast<uint16_t*>(buf), 2);

    kfree(buf);
}

bool ext2::read_block(ext2_fs_t* fs, const uint32_t block_num, uint8_t* buffer, const uint32_t blocks_to_read) {
    // Translating Ext2 blocks to LBA blocks
    uint32_t lba = fs->partition_start + ((block_num * fs->block_size) / 512);
    uint32_t lba_blocks = (blocks_to_read * fs->block_size) / 512;

    if (fs->dev) {
        // ATA driver expects words, so cast here only
        pio_28::read_sector(fs->dev, lba, reinterpret_cast<uint16_t*>(buffer), lba_blocks);
    }
    else {
        vga::error("Ext2 FS doesn't have a device!\n");
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
        vga::error("Ext2 FS doesn't have a device!\n");
        return false;
    }
    return true;
}


ext2_fs_t* curr_fs = nullptr;
// Parses dir entries in a block and adds them to a list
void parse_directory_block(ext2_fs_t* fs, uint8_t* block, data::list<vfsNode>& entries, treeNode* parentNode, vfsNode parent) {
    uint32_t offset = 0;

    while (offset < fs->block_size) {
        dir_ent_t* entry = (dir_ent_t*)(block + offset);
        if (entry->inode == 0) break;
        if (entry->entry_size < 8) break; // Corrupted or too small entry

        inode_t* inode = ext2::load_inode(fs, entry->inode);

        data::string name(entry->name, entry->name_len);
        if (!inode) vga::warning("parse_directory_block: No inode for %S\n", name);

        // Build full path
        data::string path = parent.path;
        path.append(name);

        if (INODE_IS_DIR(inode)) path.append("/");

        // Create VFS node
        vfsNode node = vfs_tree.create({name, path, INODE_IS_DIR(inode), entry->inode, inode, curr_fs})->data;

        // Add to VFS tree if not '.' or '..'
        if (!name.equals(".") && !name.equals("..")) {
            vfs::add_node(parentNode, node);
        }

        // Add to dynamic list
        entries.push_back(node);

        offset += entry->entry_size;
    }
}

// Returns a list of VFS nodes of entries in the given directory
data::list<vfsNode> ext2::read_dir(treeNode* tree_node) {
    data::list<vfsNode> entries;
    curr_fs = tree_node->data.fs;

    if (!tree_node) return entries;

    vfsNode node = tree_node->data;
    if (!node.is_dir) {
        vga::error("read_dir error: node is not a directory!\n");
        return entries;
    }

    // If we don't have a physical FS, gather children from VFS tree
    if (!curr_fs) {
        treeNode* curr = tree_node->first_child;
        while (curr) {
            entries.push_back(curr->data);
            curr = curr->next_sibling;
        }
        return entries;
    }

    auto read_and_parse = [&](uint32_t block_num) {
        if (block_num == 0) return;
        uint8_t block[curr_fs->block_size];
        ext2::read_block(curr_fs, block_num, block);
        parse_directory_block(curr_fs, block, entries, tree_node, node);
    };

    // ---------------------
    // Direct blocks
    // ---------------------
    for (int i = 0; i < 12; i++) {
        read_and_parse(node.inode->direct_blk_ptr[i]);
    }

    // ---------------------
    // Singly indirect block
    // ---------------------
    if (node.inode->singly_inderect_blk_ptr != 0) {
        uint32_t* ptrs = (uint32_t*)kmalloc(curr_fs->block_size);
        ext2::read_block(curr_fs, node.inode->singly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(ptrs));
        for (int i = 0; i < curr_fs->block_size / 4; ++i) {
            read_and_parse(ptrs[i]);
        }
        kfree(ptrs);
    }

    // ---------------------
    // Doubly indirect block
    // ---------------------
    if (node.inode->doubly_inderect_blk_ptr != 0) {
        uint32_t* dptrs = (uint32_t*)kmalloc(curr_fs->block_size);
        ext2::read_block(curr_fs, node.inode->doubly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(dptrs));

        for (int i = 0; i < curr_fs->block_size / 4; ++i) {
            if (dptrs[i] == 0) continue;

            uint32_t* sptrs = (uint32_t*)kmalloc(curr_fs->block_size);
            ext2::read_block(curr_fs, dptrs[i], reinterpret_cast<uint8_t*>(sptrs));

            for (int j = 0; j < curr_fs->block_size / 4; ++j) {
                read_and_parse(sptrs[j]);
            }

            kfree(sptrs);
        }

        kfree(dptrs);
    }

    // ---------------------
    // Triply indirect block
    // ---------------------
    if (node.inode->triply_inderect_blk_ptr != 0) {
        uint32_t* tptrs = (uint32_t*)kmalloc(curr_fs->block_size);
        ext2::read_block(curr_fs, node.inode->triply_inderect_blk_ptr, reinterpret_cast<uint8_t*>(tptrs));

        for (int i = 0; i < curr_fs->block_size / 4; ++i) {
            if (tptrs[i] == 0) continue;

            uint32_t* dptrs = (uint32_t*)kmalloc(curr_fs->block_size);
            ext2::read_block(curr_fs, tptrs[i], reinterpret_cast<uint8_t*>(dptrs));

            for (int j = 0; j < curr_fs->block_size / 4; ++j) {
                if (dptrs[j] == 0) continue;

                uint32_t* sptrs = (uint32_t*)kmalloc(curr_fs->block_size);
                ext2::read_block(curr_fs, dptrs[j], reinterpret_cast<uint8_t*>(sptrs));

                for (int k = 0; k < curr_fs->block_size / 4; ++k) {
                    read_and_parse(sptrs[k]);
                }

                kfree(sptrs);
            }

            kfree(dptrs);
        }

        kfree(tptrs);
    }

    return entries;
}


#pragma endregion

#pragma region Init

// Initializes the Ext2 FS for an ATA device
ext2_fs_t* ext2::init_ext2_device(ata::device_t* dev, bool sysdisk_check) {
    // Getting MBR
    mbr_t* mbr = (mbr_t*)kmalloc(sizeof(mbr_t));
    mbr::read_mbr(dev, mbr);
    uint32_t partition_start = mbr::find_partition_lba(mbr);
    
    // Allocating space for the FS metadata and superblock. Adding device 
    ext2_fs_t* ext2fs = (ext2_fs_t*)kcalloc(1, sizeof(ext2_fs_t));
    ext2fs->dev = dev;
    ext2fs->partition_start = partition_start;
    ext2fs->sb = (superblock_t*)kmalloc(SUPERBLOCK_SIZE);
    
    // Reading superblock (Located at LBA 2 and takes up 2 sectors)
    pio_28::read_sector(dev, partition_start + 2, (uint16_t*)ext2fs->sb, 2);
    
    // Verifying superblock
    if(ext2fs->sb->ext2_magic != EXT2_MAGIC) {
        // Mounting to VFS
        if(!sysdisk_check) vfs::mount_dev(vfs::ide_device_names[vfs::ide_device_name_index++], nullptr, nullptr);
        return nullptr;
    }
    // Saving information about device
    ext2fs->block_size = 1024 << ext2fs->sb->blk_size;
    ext2fs->blocks_per_group = ext2fs->sb->blkgroup_blk_num;
    ext2fs->inodes_per_group = ext2fs->sb->blkgroup_inode_num;
    
    // Total number of block groups (rounded up)
    ext2fs->total_groups = 
    (ext2fs->sb->blks_num + ext2fs->blocks_per_group - 1) / ext2fs->blocks_per_group;
    
    // Number of blocks needed for the Block Group Descriptor Table (rounded up)
    ext2fs->blk_grp_desc_blocks = 
    (ext2fs->total_groups * sizeof(blkgrp_descriptor_t) + ext2fs->block_size - 1) / ext2fs->block_size;
    
    // Allocating and reading BGD
    ext2fs->blk_grp_descs = (blkgrp_descriptor_t*)kmalloc(ext2fs->blk_grp_desc_blocks * ext2fs->block_size);
    uint32_t bgd_start = (ext2fs->block_size == 1024) ? 2 : 1;
    ext2::read_block(ext2fs, bgd_start, reinterpret_cast<uint8_t*>(ext2fs->blk_grp_descs), ext2fs->blk_grp_desc_blocks);
    
    // Getting root inode
    inode_t* root_inode = load_inode(ext2fs, EXT2_ROOT_INO);
    if (!INODE_IS_DIR(root_inode)) {
        // Not a directory, something went wrong
        vga::error("The root inode wasn't a directory for ATA device!\n");
        // Mounting to VFS
        if(!sysdisk_check) vfs::mount_dev(vfs::ide_device_names[vfs::ide_device_name_index++], nullptr, nullptr);
        return nullptr;
    }

    // Mounting to VFS
    if(!sysdisk_check) vfs::mount_dev(vfs::ide_device_names[vfs::ide_device_name_index++], root_inode, ext2fs);

    return ext2fs;
}

// Finds all Ext2 File Systems
void ext2::find_ext2_fs(void) {
    // Initializing Ext2 on ATA devices
    for(int i = 0; i < last_ata_device_index; i++)
        if(ata_devices[i]) {
            ext2::init_ext2_device(ata_devices[i], false);
        }

    // TODO: Find on AHCI when implemented
}

#pragma endregion

#pragma region Helper Functions

/// @brief Clears a specific bit in a bitmap (marks block/inode free)
/// @param bitmap Pointer to bitmap in memory
/// @param bit Bit index to clear
void ext2::clear_bitmap_bit(uint8_t* bitmap, uint32_t bit) {
    if(!bitmap) return;
    uint32_t byte_index = bit / 8;
    uint8_t bit_index = bit % 8;
    bitmap[byte_index] &= ~(1 << bit_index); // clear the bit
}

static inline uint16_t align4_u16(uint16_t x) { return (uint16_t)((x + 3u) & ~3u); }
static inline uint16_t dirent_rec_len(uint8_t name_len) { return (uint16_t)(8u + align4_u16(name_len)); }
static inline void zero_block(ext2_fs_t* fs, uint32_t blk, uint32_t block_size){
    uint8_t* z = (uint8_t*)kcalloc(1, block_size);
    ext2::write_block(fs, blk, z);
    kfree(z);
}

/// @brief Transforms the type and permissions field of an inode into a readable permission string
/// @param mode Type and permissions field of an inode
/// @return Readable string permission
data::string ext2::mode_to_string(const uint16_t mode) {
    data::string str;

    // File type
    switch ((mode & EXT2_S_IFMT)) {
        case EXT2_S_IFREG: str.append("-"); break;
        case EXT2_S_IFDIR: str.append("d"); break;
        case EXT2_S_IFLNK: str.append("l"); break;
        case EXT2_S_IFCHR: str.append("c"); break;
        case EXT2_S_IFBLK: str.append("b"); break;
        case EXT2_S_IFIFO: str.append("p"); break;
        case EXT2_S_IFSOCK: str.append("s"); break;
        default:           str.append("?"); break;
    }

    // Owner
    str.append((mode & EXT2_S_IRUSR) ? "r" : "-");
    str.append((mode & EXT2_S_IWUSR) ? "w" : "-");
    str.append((mode & EXT2_S_IXUSR) ? "x" : "-");

    // Group
    str.append((mode & EXT2_S_IRGRP) ? "r" : "-");
    str.append((mode & EXT2_S_IWGRP) ? "w" : "-");
    str.append((mode & EXT2_S_IXGRP) ? "x" : "-");

    // Others
    str.append((mode & EXT2_S_IROTH) ? "r" : "-");
    str.append((mode & EXT2_S_IWOTH) ? "w" : "-");
    str.append((mode & EXT2_S_IXOTH) ? "x" : "-");

    return str;
}

/// @brief Gets inodes type (e.g. Dir, file, symlink...)
/// @param inode Inode to check
/// @return Type of inode
uint8_t ext2::get_inode_type(const inode_t* inode) {
    switch (inode->type_and_perm & EXT2_S_IFMT) {
        case EXT2_S_IFREG:  return EXT2_FT_REG_FILE;
        case EXT2_S_IFDIR:  return EXT2_FT_DIR;     
        case EXT2_S_IFCHR:  return EXT2_FT_CHRDEV;  
        case EXT2_S_IFBLK:  return EXT2_FT_BLKDEV;  
        case EXT2_S_IFIFO:  return EXT2_FT_FIFO;    
        case EXT2_S_IFSOCK: return EXT2_FT_SOCK;    
        case EXT2_S_IFLNK:  return EXT2_FT_SYMLINK; 
        default:            return EXT2_FT_UNKNOWN; 
    }
}

/// @brief Gets permissions (rwx) of the current user for an inode
/// @param inode Inode we're trying to access
/// @param uid User ID
/// @param gid Group ID
/// @return Permissions read / write / execute
ext2_perms ext2::get_perms(const inode_t* inode, const uint32_t uid, const uint32_t gid) {
    if(!inode) {
        vga::warning("get_perms: Inode not found!\n");
        return {false, false, false};
    }
    
    // If user is root, it has full access automatically
    if(uid == 0) return {true, true, true};
    
    uint16_t mode = inode->type_and_perm;
    // If the user is owner
    if(inode->uid == uid) return {mode & EXT2_S_IRUSR, mode & EXT2_S_IWUSR, mode & EXT2_S_IXUSR};
    // If the user is in the inodes group
    if(inode->gid == gid) return {mode & EXT2_S_IRGRP, mode & EXT2_S_IWGRP, mode & EXT2_S_IXGRP};
    // Other user
    return {mode & EXT2_S_IROTH, mode & EXT2_S_IWOTH, mode & EXT2_S_IXOTH};
}
#pragma endregion

/// @brief cd (Change directory) logic
/// @param dir Dir to change to
/// @return If we could change to the dir
bool change_dir(data::string dir) {
    // Current dir, no need to change anything
    if(dir.equals(".")) return true;
    
    // Finding child (dir to find) from current node (VFS tree)
    treeNode* currNode = vfs::get_node(vfs::currentDir);
    // Parent dir
    if(dir.equals("..")) {
        // If we're in root
        if(!currNode->parent) {
            vga::warning("cd: No parent for root dir!\n");
            return false;
        }
        // Checking permission to change
        if(currNode->parent->data.inode && !ext2::get_perms(currNode->parent->data.inode, vfs::currUid, vfs::currGid).execute) {
            vga::warning("cd: Can't change to dir \"%S\", permission denied!\n", currNode->parent->data.path);
            return false;
        }

        // Changing FS if possible
        if(curr_fs != currNode->parent->data.fs) curr_fs = currNode->parent->data.fs;

        vfs::currentDir = currNode->parent->data.path;
        return true;
    }
    
    treeNode* found_dir = vfs_tree.find_child_by_predicate(currNode, [dir](vfsNode node){return node.name == dir;});
    // If we found in tree
    if(found_dir && found_dir->data.is_dir) {
        // Checking permission to change
        if(found_dir->data.inode && !ext2::get_perms(found_dir->data.inode, vfs::currUid, vfs::currGid).execute) {
            vga::warning("cd: Can't change to dir \"%S\", permission denied!\n", found_dir->data.path);
            return false;
        }
        vfs::currentDir = found_dir->data.path;
        // Changing FS if possible
        if(curr_fs != found_dir->data.fs) curr_fs = found_dir->data.fs;
        return true;
    }

    // If we're in virtual dirs, it's pointless to check again physically
    if (!curr_fs) {
        vga::warning("cd: Couldn't find directory \"%S\" in \"%S\"\n", dir, vfs::currentDir);
        return false;
    }
    
    // If we couldnt find the dir in the virtual tree, we'll check physically
    data::list<vfsNode> nodes = ext2::read_dir(currNode);
    if(nodes.empty()) return false;

    for(vfsNode node : nodes) {
        // If we found it here
        if(node.name == dir && node.is_dir) {
            // Checking permission to change
            if(node.inode && !ext2::get_perms(node.inode, vfs::currUid, vfs::currGid).execute) {
                vga::warning("cd: Can't change to dir \"%S\", permission denied!\n", node.path);
                return false;
            }
            vfs::currentDir = node.path;
            // Changing FS if possible
            if(curr_fs != node.fs) curr_fs = node.fs;
            
            // Adding to VFS tree
            vfs::add_node(currNode, node.name, node.inode_num, node.inode, node.fs);
            return true;
        }
    }

    vga::warning("cd: Couldn't find directory \"%S\" in \"%S\"\n", dir, vfs::currentDir);
    return false;
}

// Inserts dir entry in block
bool insert_into_block(ext2_fs_t* fs, uint32_t block_num, const char* name, uint32_t inode_num, uint8_t* buf, bool* inserted) {
    ext2::read_block(fs, block_num, buf);

    const uint16_t needed_len = dirent_rec_len((uint8_t)strlen(name));

    uint32_t offset = 0;
    // Checking block for inodes
    while (offset < fs->block_size) {
        dir_ent_t* entry = (dir_ent_t*)(buf + offset);

        if (entry->entry_size == 0) break;

        uint16_t actual_len = dirent_rec_len(entry->name_len); // Rounding up to next multiple of 4

        // If we found an empty slot
        if (entry->inode == 0) {
            if (entry->entry_size >= needed_len) {
                uint16_t orig_len = entry->entry_size;

                entry->inode = inode_num;
                entry->name_len = (uint8_t)strlen(name);
                inode_t* inode = ext2::load_inode(fs, inode_num);
                entry->type_indicator = ext2::get_inode_type(inode);
                kfree(inode);

                if (orig_len > needed_len) {
                    entry->entry_size = needed_len;

                    dir_ent_t* remainder = (dir_ent_t*)((uint8_t*)entry + needed_len);
                    remainder->inode = 0;
                    remainder->name_len = 0;
                    remainder->type_indicator = 0;
                    remainder->entry_size = (uint16_t)(orig_len - needed_len);
                } else {
                    entry->entry_size = orig_len;
                }

                memcpy(entry->name, name, entry->name_len);
                // Writing back to disk
                ext2::write_block(fs, block_num, buf);
                *inserted = true;
                return true;
            }
        }

        // If the entry has extra space we'll split it
        if (entry->entry_size > actual_len) {
            uint16_t extra = (uint16_t)(entry->entry_size - actual_len);
            if (extra >= needed_len) {
                dir_ent_t* new_entry = (dir_ent_t*)((uint8_t*)entry + actual_len);

                // Setting info
                entry->entry_size = actual_len;

                new_entry->inode = inode_num;
                new_entry->name_len = (uint8_t)strlen(name);
                inode_t* inode = ext2::load_inode(fs, inode_num);
                new_entry->type_indicator = ext2::get_inode_type(inode);
                kfree(inode);

                uint16_t new_min = dirent_rec_len(new_entry->name_len);
                if (extra > new_min) {
                    new_entry->entry_size = new_min;

                    dir_ent_t* trailing = (dir_ent_t*)((uint8_t*)new_entry + new_min);
                    trailing->inode = 0;
                    trailing->name_len = 0;
                    trailing->type_indicator = 0;
                    trailing->entry_size = (uint16_t)(extra - new_min);
                } else {
                    new_entry->entry_size = extra;
                }

                memcpy(new_entry->name, name, new_entry->name_len);
                // Writing back to disk
                ext2::write_block(fs, block_num, buf);
                *inserted = true;
                return true;
            }
        }

        // Going to next entry
        offset += entry->entry_size;
    }

    return false;
}

// Inserts a directory entry in a directory
int insert_directory_entry(ext2_fs_t* fs, inode_t* parent_inode, const char* name, uint32_t inode_num, uint8_t* buf) {
    bool inserted = false;

    // -----------------------------------
    // Handle Direct Block
    // -----------------------------------
    for (int i = 0; i < 12; i++) {
        // Allocating block if zero
        if (parent_inode->direct_blk_ptr[i] == 0){
            parent_inode->direct_blk_ptr[i] = ext2::alloc_block(fs);
            if (parent_inode->direct_blk_ptr[i] == (uint32_t)-1) return -1;
            zero_block(fs, parent_inode->direct_blk_ptr[i], fs->block_size);
            parent_inode->size_low += fs->block_size;
        }

        // If we could insert a value into a block
        if (insert_into_block(fs, parent_inode->direct_blk_ptr[i], name, inode_num, buf, &inserted))
            return 0;
    }

    const uint32_t ptrs_per_block = fs->block_size / 4;

    // -----------------------------------
    // Handle Singly Indirect Block
    // -----------------------------------
    if (parent_inode->singly_inderect_blk_ptr == 0){
        // Allocating block if zero
        parent_inode->singly_inderect_blk_ptr = ext2::alloc_block(fs);
        if (parent_inode->singly_inderect_blk_ptr == (uint32_t)-1) return -2;
        zero_block(fs, parent_inode->singly_inderect_blk_ptr, fs->block_size);
    }

    uint32_t* singly = (uint32_t*)kcalloc(1, fs->block_size);
    ext2::read_block(fs, parent_inode->singly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(singly));

    for (uint32_t i = 0; i < ptrs_per_block; i++) {
        if (singly[i] == 0){
            // Allocating block if zero
            singly[i] = ext2::alloc_block(fs);
            if (singly[i] == (uint32_t)-1){ kfree(singly); return -2; }
            zero_block(fs, singly[i], fs->block_size);
            parent_inode->size_low += fs->block_size;
        }

        // If we could insert a value into a block
        if (insert_into_block(fs, singly[i], name, inode_num, buf, &inserted)) {
            ext2::write_block(fs, parent_inode->singly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(singly));
            kfree(singly);
            return 0;
        }
    }
    ext2::write_block(fs, parent_inode->singly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(singly)); // Persist pointer updates
    kfree(singly);

    // -----------------------------------
    // Handle Doubly Indirect Block
    // -----------------------------------
    if (parent_inode->doubly_inderect_blk_ptr == 0){
        parent_inode->doubly_inderect_blk_ptr = ext2::alloc_block(fs);
        if (parent_inode->doubly_inderect_blk_ptr == (uint32_t)-1) return -2;
        zero_block(fs, parent_inode->doubly_inderect_blk_ptr, fs->block_size);
    }

    uint32_t* doubly = (uint32_t*)kcalloc(1, fs->block_size);
    ext2::read_block(fs, parent_inode->doubly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(doubly));

    for (uint32_t i = 0; i < ptrs_per_block; i++) {
        if (doubly[i] == 0){
            // Allocating block if zero (level-1 index)
            doubly[i] = ext2::alloc_block(fs);
            if (doubly[i] == (uint32_t)-1){ kfree(doubly); return -2; }
            zero_block(fs, doubly[i], fs->block_size);
        }

        uint32_t* singly_lvl2 = (uint32_t*)kcalloc(1, fs->block_size);
        ext2::read_block(fs, doubly[i], reinterpret_cast<uint8_t*>(singly_lvl2));

        for (uint32_t j = 0; j < ptrs_per_block; j++) {
            if (singly_lvl2[j] == 0){
                // Allocating block if zero
                singly_lvl2[j] = ext2::alloc_block(fs);
                if (singly_lvl2[j] == (uint32_t)-1){ kfree(singly_lvl2); kfree(doubly); return -2; }
                zero_block(fs, singly_lvl2[j], fs->block_size);
                parent_inode->size_low += fs->block_size;
            }

            // If we could insert a value into a block
            if (insert_into_block(fs, singly_lvl2[j], name, inode_num, buf, &inserted)) {
                ext2::write_block(fs, doubly[i], reinterpret_cast<uint8_t*>(singly_lvl2));
                ext2::write_block(fs, parent_inode->doubly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(doubly));
                kfree(singly_lvl2);
                kfree(doubly);
                return 0;
            }
        }
        ext2::write_block(fs, doubly[i], reinterpret_cast<uint8_t*>(singly_lvl2));
        kfree(singly_lvl2);
    }
    ext2::write_block(fs, parent_inode->doubly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(doubly));
    kfree(doubly);

    // -----------------------------------
    // Handle Triply Indirect Block
    // -----------------------------------
    if (parent_inode->triply_inderect_blk_ptr == 0){
        parent_inode->triply_inderect_blk_ptr = ext2::alloc_block(fs);
        if (parent_inode->triply_inderect_blk_ptr == (uint32_t)-1) return -2;
        zero_block(fs, parent_inode->triply_inderect_blk_ptr, fs->block_size);
    }

    uint32_t* triply = (uint32_t*)kcalloc(1, fs->block_size);
    ext2::read_block(fs, parent_inode->triply_inderect_blk_ptr, reinterpret_cast<uint8_t*>(triply));

    for (uint32_t i = 0; i < ptrs_per_block; i++) {
        if (triply[i] == 0){
            triply[i] = ext2::alloc_block(fs);
            if (triply[i] == (uint32_t)-1){ kfree(triply); return -2; }
            zero_block(fs, triply[i], fs->block_size);
        }

        uint32_t* doubly_lvl2 = (uint32_t*)kcalloc(1, fs->block_size);
        ext2::read_block(fs, triply[i], reinterpret_cast<uint8_t*>(doubly_lvl2));

        for (uint32_t j = 0; j < ptrs_per_block; j++) {
            if (doubly_lvl2[j] == 0){
                doubly_lvl2[j] = ext2::alloc_block(fs);
                if (doubly_lvl2[j] == (uint32_t)-1){ kfree(doubly_lvl2); kfree(triply); return -2; }
                zero_block(fs, doubly_lvl2[j], fs->block_size);
            }

            uint32_t* singly_lvl3 = (uint32_t*)kcalloc(1, fs->block_size);
            ext2::read_block(fs, doubly_lvl2[j], reinterpret_cast<uint8_t*>(singly_lvl3));

            for (uint32_t k = 0; k < ptrs_per_block; k++) {
                if (singly_lvl3[k] == 0){
                    // Allocating block if zero
                    singly_lvl3[k] = ext2::alloc_block(fs);
                    if (singly_lvl3[k] == (uint32_t)-1){ kfree(singly_lvl3); kfree(doubly_lvl2); kfree(triply); return -2; }
                    zero_block(fs, singly_lvl3[k], fs->block_size);
                    parent_inode->size_low += fs->block_size;
                }

                // If we could insert a value into a block
                if (insert_into_block(fs, singly_lvl3[k], name, inode_num, buf, &inserted)) {
                    ext2::write_block(fs, doubly_lvl2[j], reinterpret_cast<uint8_t*>(singly_lvl3));
                    ext2::write_block(fs, triply[i], reinterpret_cast<uint8_t*>(doubly_lvl2));
                    ext2::write_block(fs, parent_inode->triply_inderect_blk_ptr, reinterpret_cast<uint8_t*>(triply));
                    kfree(singly_lvl3);
                    kfree(doubly_lvl2);
                    kfree(triply);
                    return 0;
                }
            }
            ext2::write_block(fs, doubly_lvl2[j], reinterpret_cast<uint8_t*>(singly_lvl3));
            kfree(singly_lvl3);
        }
        ext2::write_block(fs, triply[i], reinterpret_cast<uint8_t*>(doubly_lvl2));
        kfree(doubly_lvl2);
    }
    ext2::write_block(fs, parent_inode->triply_inderect_blk_ptr, reinterpret_cast<uint8_t*>(triply));
    kfree(triply);

    return -3; // No space found
}

/// @brief Removes a dir entry, supports up to triple indirect blocks
/// @param fs File system
/// @param parent_inode Parent directories inode
/// @param name Name of entry to remove
void remove_dir_entry(ext2_fs_t* fs, inode_t* parent_inode, data::string name, uint8_t type) {
    if (!parent_inode) return;

    auto scan_block = [&](uint32_t block_num) -> bool {
        if (block_num == 0) return false;
        uint8_t* block = (uint8_t*)kmalloc(fs->block_size);
        ext2::read_block(fs, block_num, block);

        uint32_t offset = 0;
        dir_ent_t* prev = nullptr;
        while (offset < fs->block_size) {
            dir_ent_t* entry = (dir_ent_t*)(block + offset);

            if (entry->inode != 0) {
                data::string entry_name(entry->name, entry->name_len);

                if (entry_name == name && entry->type_indicator == type) {
                    if (prev) {
                        prev->entry_size += entry->entry_size; // merge
                    } else {
                        entry->inode = 0; // clear if first
                    }
                    ext2::write_block(fs, block_num, block);
                    kfree(block);
                    return true;
                }
            }
            prev = entry;
            offset += entry->entry_size;
        }
        kfree(block);
        return false;
    };

    // --- Direct blocks ---
    for (int i = 0; i < 12; i++) {
        if (scan_block(parent_inode->direct_blk_ptr[i])) return;
    }

    // --- Single indirect ---
    if (parent_inode->singly_inderect_blk_ptr) {
        uint32_t* table = (uint32_t*)kmalloc(fs->block_size);
        ext2::read_block(fs, parent_inode->singly_inderect_blk_ptr, (uint8_t*)table);
        uint32_t per_block = fs->block_size / sizeof(uint32_t);
        for (uint32_t i = 0; i < per_block; i++) {
            if (scan_block(table[i])) {
                kfree(table);
                return;
            }
        }
        kfree(table);
    }

    // --- Double indirect ---
    if (parent_inode->doubly_inderect_blk_ptr) {
        uint32_t* lvl1 = (uint32_t*)kmalloc(fs->block_size);
        ext2::read_block(fs, parent_inode->doubly_inderect_blk_ptr, (uint8_t*)lvl1);
        uint32_t per_block = fs->block_size / sizeof(uint32_t);
        for (uint32_t i = 0; i < per_block; i++) {
            if (lvl1[i] == 0) continue;
            uint32_t* lvl2 = (uint32_t*)kmalloc(fs->block_size);
            ext2::read_block(fs, lvl1[i], (uint8_t*)lvl2);
            for (uint32_t j = 0; j < per_block; j++) {
                if (scan_block(lvl2[j])) {
                    kfree(lvl2);
                    kfree(lvl1);
                    return;
                }
            }
            kfree(lvl2);
        }
        kfree(lvl1);
    }

    // --- Triple indirect ---
    if (parent_inode->triply_inderect_blk_ptr) {
        uint32_t* lvl1 = (uint32_t*)kmalloc(fs->block_size);
        ext2::read_block(fs, parent_inode->triply_inderect_blk_ptr, (uint8_t*)lvl1);
        uint32_t per_block = fs->block_size / sizeof(uint32_t);

        for (uint32_t i = 0; i < per_block; i++) {
            if (lvl1[i] == 0) continue;
            uint32_t* lvl2 = (uint32_t*)kmalloc(fs->block_size);
            ext2::read_block(fs, lvl1[i], (uint8_t*)lvl2);

            for (uint32_t j = 0; j < per_block; j++) {
                if (lvl2[j] == 0) continue;
                uint32_t* lvl3 = (uint32_t*)kmalloc(fs->block_size);
                ext2::read_block(fs, lvl2[j], (uint8_t*)lvl3);

                for (uint32_t k = 0; k < per_block; k++) {
                    if (scan_block(lvl3[k])) {
                        kfree(lvl3);
                        kfree(lvl2);
                        kfree(lvl1);
                        return;
                    }
                }
                kfree(lvl3);
            }
            kfree(lvl2);
        }
        kfree(lvl1);
    }
}


/// @brief Removes a directory entry
/// @param node_to_remove Node to remove
void remove_entry(treeNode* node_to_remove) {
    vfsNode parent_node = node_to_remove->parent->data;
    vfsNode node = node_to_remove->data;
    if(!node_to_remove || node.path.empty() || !node.fs || !node.inode) {
        vga::warning("rm: Invalid node passed to remove_entry!\n");
        return;
    }
    // Reading dir entries to add any missing entries of current dir to the VFS tree
    if(node_to_remove->data.is_dir) { int count; ext2::read_dir(node_to_remove); }

    // Checking permissions to delete
    inode_t* inode_to_check = node.is_dir ? node.inode : parent_node.inode; // If it is a file, we'll check the parent dirs inode
    ext2_perms perms = ext2::get_perms(inode_to_check, vfs::currUid, vfs::currGid);
    if(!(perms.write && perms.execute)) { // We need -wx at least
        vga::warning("rm: Permission denied to delete \"%S\"\n", node.name);
        return;
    }

    // Removing dir entry of what we want deleted
    remove_dir_entry(parent_node.fs, parent_node.inode, node.name, ext2::get_inode_type(node.inode));

    // Removing hard links
    if (node.is_dir) {
        // Directory removal
        node.inode->hard_link_count--;        // Parent link
        parent_node.inode->hard_link_count--;    // ".." in child no longer points to parent
    } else {
        // File removal
        node.inode->hard_link_count--;           // Just the file itself
    }

    // If no more links exist we'll free the inode and blocks
    if(node.inode->hard_link_count == 0) {
        ext2::free_inode(node.fs, node.inode_num);
        ext2::free_blocks(node.fs, node.inode);
    }

    // Rewriting Superblock and Block Group Descriptors to account for the new data/bitmaps
    ext2::rewrite_sb(node.fs);
    ext2::rewrite_bgds(node.fs);
    // Rewriting parent inode
    ext2::write_inode(parent_node.fs, parent_node.inode_num, parent_node.inode);
} 

#pragma region Terminal Functions

/// @brief Prints working directory
void ext2::pwd(data::list<data::string> params) {
    vga::printf("%S\n", vfs::currentDir);
}

/// @brief Lists directory entries
void ext2::ls(data::list<data::string> params) {
    bool metadata_print = false;
    if(params.count() >= 1) {
        if(params.at(0).equals("-l")) metadata_print = true;
        else {
            vga::warning("ls: Invalid parameter \"%S\" passed to ls\n", params.at(0));
            return;
        }
    }

    treeNode* node = vfs::get_node(vfs::currentDir); // Retreiving node from current path
    if(!node) return;
    data::list<vfsNode> nodes = ext2::read_dir(node);
    // Printing
    // Default listing
    if(!metadata_print) {
        for(vfsNode node : nodes) {
            // Permission denied
            if(node.inode && !ext2::get_perms(node.inode, vfs::currUid, vfs::currGid).read) continue;
            // Printing all entries instead of parent and same dir
            if(!node.name.equals(".") && !node.name.equals("..")) {
                vga::printf(node.is_dir ? PRINT_COLOR_LIGHT_BLUE | (PRINT_COLOR_BLACK << 4) : PRINT_COLOR_WHITE | (PRINT_COLOR_BLACK << 4), "%S ", node.name);
            }
        }
        vga::printf("\n");
    }
    // Metadata printing
    else 
        for(vfsNode node : nodes) {
            // Permission denied
            if(node.inode && !ext2::get_perms(node.inode, vfs::currUid, vfs::currGid).read) continue;
            // Printing all entries instead of parent and same dir
            if(!node.name.equals(".") && !node.name.equals("..")) {
                if(node.inode) {
                    // Type and permissions
                    vga::printf("%S ", mode_to_string(node.inode->type_and_perm));
                    // Link counts
                    vga::printf("%u ", node.inode->hard_link_count);
                    // UID
                    vga::printf("%u ", node.inode->uid);
                    // GID
                    vga::printf("%u ", node.inode->gid);
                    // Size
                    vga::printf("%u ", node.inode->size_low);
                    // Inode num
                    vga::printf("%u ", node.inode_num);
                    // Modify time
                    vga::printf("%S ", rtc::timestamp_to_string(node.inode->last_mod_time));
                }
                else vga::printf("(Couldn't recognize File System / Inode) ");
                // Name
                vga::printf(node.is_dir ? PRINT_COLOR_LIGHT_BLUE | (PRINT_COLOR_BLACK << 4) : PRINT_COLOR_WHITE | (PRINT_COLOR_BLACK << 4), "%S\n", node.name);
            }
        }
}


/// @brief Changes directory
void ext2::cd(data::list<data::string> params) {
    // Getting directory to change to
    data::string input = params.at(0);
    if(input.empty()) {
        vga::warning("cd: Syntax: cd <dir>\n");
        return;
    }

    int count;
    data::string* dirs = split_path_tokens(input, count);
    // Calling logic for all dirs
    for(int i = 0; i < count; i++)
        if(!change_dir(dirs[i])) return;
}


/// @brief Created a directory in a FS 
void ext2::mkdir(data::list<data::string> params) {
    data::string dir = params.at(0);
    if(dir.empty()) {
        vga::warning("mkdir: Syntax: mkdir <dir>\n");
        return;
    }
    if(dir.includes("/")) {
        vga::warning("mkdir: Please don't use '/' in a directory name\n");
        return;
    }

    treeNode* node = vfs::get_node(vfs::currentDir); // Retreiving node from current path
    if(!node) return;
    vfsNode parent = node->data;
    if(!parent.fs) return;

    ext2::make_dir(dir, parent, node, DEFAULT_PERMS);
}

void ext2::make_dir(data::string dir, vfsNode parent, treeNode* node, uint16_t perms) {
    ext2_fs_t* fs = parent.fs;
    // Checking if we're in a Ext2 FS to create a dir
    if(!fs) {
        vga::warning("mkdir: Can't create directory in \"%S\", because it's not in an Ext2 File System!\n", parent.path);
        return;
    }
    // Permission denied
    if(parent.inode && !ext2::get_perms(parent.inode, vfs::currUid, vfs::currGid).write) {
        vga::warning("mkdir: Can't create directory in \"%S\", permission denied!\n", parent.path);
        return;
    }

    data::list<vfsNode> nodes = ext2::read_dir(node);
    // Checking if the dir already exists
    for(vfsNode node : nodes) {
        // Checking if this node is the dir we want to create
        if(node.name == dir && node.is_dir) {
            vga::warning("mkdir: Directory \"%S\" already exists in \"%S\"\n", dir, parent.path);
            return;
        }
    }

    // Allocating inode and block
    uint32_t inode_num = ext2::alloc_inode(fs);
    if(inode_num == (uint32_t)-1) {
        vga::error("mkdir: Couldn't create directory do to inode allocation fail!\n");
        return;
    }
    uint32_t block_num = ext2::alloc_block(fs);
    if(block_num == (uint32_t)-1) {
        vga::error("mkdir: Couldn't create directory do to block allocation fail!\n");
        return;
    }
    
    // Getting and initializing newly allocated inode
    inode_t* inode = load_inode(fs, inode_num);
    memset(inode, 0, sizeof(inode_t));
    
    inode->type_and_perm = EXT2_S_IFDIR | perms;
    inode->uid = vfs::currUid;
    inode->gid = vfs::currGid;
    inode->size_low = fs->block_size;
    inode->create_time = inode->last_access_time = inode->last_mod_time = rtc::get_unix_timestamp();
    inode->hard_link_count = 2;  // '.' and '..'
    inode->disk_sect_count = (fs->block_size / 512);
    inode->direct_blk_ptr[0] = block_num;

    uint8_t* buf = (uint8_t*)kcalloc(1, fs->block_size);

    // Create directory entries: '.' and '..'
    dir_ent_t* dot = (dir_ent_t*)buf;
    dot->inode = inode_num;
    dot->name_len = 1;
    dot->type_indicator = EXT2_FT_DIR;
    dot->name[0] = '.';
    dot->entry_size = dirent_rec_len(dot->name_len);

    dir_ent_t* dotdot = (dir_ent_t*)((uint8_t*)dot + dot->entry_size);
    dotdot->inode = parent.inode_num;
    dotdot->name_len = 2;
    dotdot->type_indicator = EXT2_FT_DIR;
    dotdot->name[0] = '.';
    dotdot->name[1] = '.';
    dotdot->entry_size = (uint16_t)(fs->block_size - dot->entry_size); // Rest of block

    // Writing to disk
    write_block(fs, block_num, buf);
    write_inode(fs, inode_num, inode);

    // Insert into parent dir
    int res = insert_directory_entry(fs, parent.inode, dir, inode_num, buf);
    if (res != 0) {
        kfree(buf);
        return;
    }

    parent.inode->hard_link_count++;
    parent.inode->last_mod_time = rtc::get_unix_timestamp();

    // Write back inodes
    write_inode(fs, parent.inode_num, parent.inode);
    inode = load_inode(fs, inode_num);

    // Adding to VFS
    vfs::add_node(node, dir, inode_num, inode, fs);

    kfree(buf);
    return;
}

/// @brief Makes a file
void ext2::mkfile(data::list<data::string> params) {
    data::string file = params.at(0);
    if(file.empty()) {
        vga::warning("mkfile: Syntax: mkfile <file>\n");
        return;
    }
    if(file.includes("/")) {
        vga::warning("mkfile: Please don't use '/' in a file name\n");
        return;
    }

    treeNode* node = vfs::get_node(vfs::currentDir); // Retreiving node from current path
    if(!node) return;
    vfsNode parent = node->data;
    if(!parent.fs) return;

    ext2::make_file(file, parent, node, DEFAULT_PERMS);
}

void ext2::make_file(data::string file, vfsNode parent, treeNode* node, uint16_t perms) {
    ext2_fs_t* fs = parent.fs;
    // Checking if we're in a Ext2 FS to create a file
    if(!fs) {
        vga::warning("mkfile: Can't create file in \"%S\", because it's not in an Ext2 File System!\n", parent.path);
        return;
    }
    // Permission denied
    if(parent.inode && !ext2::get_perms(parent.inode, vfs::currUid, vfs::currGid).write) {
        vga::warning("mkfile: Can't create file in \"%S\", permission denied!\n", parent.path);
        return;
    }

    data::list<vfsNode> nodes = ext2::read_dir(node);
    // Checking if the file already exists
    for(vfsNode node : nodes) {
        // Checking if this node is the file we want to create
        if(node.name == file && !node.is_dir) {
            vga::warning("mkfile: File \"%S\" already exists in \"%S\"\n", file, parent.path);
            return;
        }
    }

    // Allocating inode and block
    uint32_t inode_num = ext2::alloc_inode(fs);
    if(inode_num == (uint32_t)-1) {
        vga::error("mkfile: Couldn't create file do to inode allocation fail!\n");
        return;
    }
    
    // Getting and initializing newly allocated inode
    inode_t* inode = load_inode(fs, inode_num);
    memset(inode, 0, sizeof(inode_t));
    
    inode->type_and_perm = EXT2_S_IFREG | perms;
    inode->uid = vfs::currUid;
    inode->gid = vfs::currGid;
    inode->size_low = 0;
    inode->create_time = inode->last_access_time = inode->last_mod_time = rtc::get_unix_timestamp();
    inode->hard_link_count = 1;
    inode->disk_sect_count = 0;

    write_inode(fs, inode_num, inode);

    uint8_t* buf = (uint8_t*)kcalloc(1, fs->block_size);
    // Insert into parent dir
    int res = insert_directory_entry(fs, parent.inode, file, inode_num, buf);
    if (res != 0) {
        kfree(buf);
        return;
    }

    parent.inode->last_mod_time = rtc::get_unix_timestamp();

    // Write back inodes
    write_inode(fs, parent.inode_num, parent.inode);
    inode = load_inode(fs, inode_num);

    // Adding to VFS
    vfs::add_node(node, file, inode_num, inode, fs);

    kfree(buf);
    return;
}

/// @brief Removes dir entry
void ext2::rm(data::list<data::string> params) {
    // Getting params
    treeNode* parent = vfs::get_node(vfs::currentDir);

    if(params.count() == 2) {
        if(params.at(0) != "-r") goto invalid_params;

        // Reading dir to find the object we want to remove
        ext2::read_dir(parent);
        data::string name = params.at(1);
        int count; treeNode** nodes = vfs_tree.find_children_by_predicate(parent, [name](vfsNode node){ return node.name == name;}, count);
        params.clear();
        if(!nodes || count == 0) {
            vga::warning("rm: Couldn't find dir \"%S\" in \"%S\"\n", name, vfs::currentDir);
            return;
        }
        for(int i = 0; i < count; i++) {
            // Found a valid node
            if(nodes[i]->data.is_dir) {
                // rm -r <dir>
                // Traversing directory recursively and deleting all contents
                vfs_tree.traverse(nodes[i], remove_entry);
                // Removing from VFS tree
                vfs_tree.delete_subtree(nodes[i]);
                return;
            }
        }
        vga::warning("rm: The object (\"%S\") to delete is a file! Please use rm <file>\n", name);
        return;
    }
    else if(params.count() == 1) {
        // Reading dir to find the object we want to remove
        ext2::read_dir(parent);
        data::string name = params.at(0);
        int count; treeNode** nodes = vfs_tree.find_children_by_predicate(parent, [name](vfsNode node){ return node.name == name; }, count);
        params.clear();
        if(!nodes || count == 0) {
            vga::warning("rm: Couldn't find file \"%S\" in \"%S\"\n", name, vfs::currentDir);
            return;
        }
        for(int i = 0; i < count; i++) {
            // Found a valid node
            if(!nodes[i]->data.is_dir) {
                // rm <file>
                remove_entry(nodes[i]);
                // Removing from VFS tree
                vfs_tree.delete_subtree(nodes[i]);
                return;
            }
        }
        
        // We need to recursively delete a directory
        vga::warning("rm: The object (\"%S\") to delete is a directory! Please use rm -r <dir>\n", name);
        return;
    }

    invalid_params:
    vga::warning("rm: Invalid parameters passed to rm!\n");
    vga::printf("rm <file> - Deletes file (doesn't work with directories)\n");
    vga::printf("rm -r <dir> - Deletes directory (recursively deletes contents)\n");
    params.clear();
    return;
}

/// @brief Prints if inode is allocated or free
void ext2::check_inode_status(data::list<data::string> params) {
    if(params.empty()) {
        vga::warning("istat: Syntax: istat <inode_num>");
        return;
    }
    uint32_t inode_num = str_to_int(params.at(0));
    uint8_t* inode_bitmap = ext2::get_inode_bitmap(curr_fs, (inode_num - 1) / curr_fs->inodes_per_group);

    if (!inode_bitmap) {
        vga::error("Error: inode bitmap not provided!\n");
        return;
    }

    // In ext filesystems, inode numbers start from 1
    uint32_t allocated = TEST_BIT(inode_bitmap, inode_num - 1);

    if (allocated)
        vga::printf("Inode %u is ALLOCATED.\n", inode_num);
    else
        vga::printf("Inode %u is FREE.\n", inode_num);
}

/// @brief Prints if inode is allocated or free
bool ext2::check_inode_status(uint32_t inode_num) {
    uint8_t* inode_bitmap = ext2::get_inode_bitmap(curr_fs, (inode_num - 1) / curr_fs->inodes_per_group);

    if (!inode_bitmap) {
        vga::error("Error: inode bitmap not provided!\n");
        return false;
    }

    // In ext filesystems, inode numbers start from 1
    return TEST_BIT(inode_bitmap, inode_num - 1);
}

#pragma endregion
