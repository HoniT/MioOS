// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// inode.cpp
// Sets up EXT2 inode functions
// ========================================

#include <fs/ext/inode.hpp>
#include <fs/ext/block.hpp>
#include <fs/ext/vfs.hpp>
#include <interrupts/kernel_panic.hpp>
#include <drivers/vga_print.hpp>
#include <mm/heap.hpp>
#include <lib/mem_util.hpp>
#include <lib/path_util.hpp>

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
    // Copying into inode to not waste memory
    inode_t* inode = (inode_t*)kmalloc(sizeof(inode_t));
    memcpy(inode, buffer + offset_in_block, sizeof(inode_t));
    kfree(buffer);
    return inode;
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
    kfree(inode_bitmap);
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
        return; // Allocation failure
    }

    ext2::read_block(fs, block_num, buf);

    // Copying given info to buffer
    // Copy only inode_size (truncate or pad relative to sizeof(inode_t))
    uint32_t copy_size = (sizeof(inode_t) < inode_size) ? sizeof(inode_t) : inode_size;
    memcpy(buf + byte_offset, inode, copy_size);

    // Rewriting block
    ext2::write_block(fs, block_num, buf);

    kfree(buf);
    return; // Success
}


/// @brief Get's inode number from path
/// @param fs Ext2 File system
/// @param path Path of node
/// @return Inode num
uint32_t ext2::find_inode(ext2_fs_t* fs, data::string path) {
    if (path.empty()) return EXT2_BAD_INO;

    int count;
    data::string* tokens = split_path_tokens(path, count);
    if (!tokens || count == 0 || !tokens[0].equals("/")) return EXT2_BAD_INO;

    treeNode* curr = vfs_tree.get_root();
    for (int i = 1; i < count; i++) {
        treeNode* parent = curr;
        bool read_dir = false;

    check:
        data::string name_to_find = tokens[i];
        if (parent->data.is_dir) {
            ext2::read_dir(parent);
        }

        curr = vfs_tree.find_child_by_predicate(parent, [name_to_find](vfsNode node) { 
            return node.name == name_to_find; 
        });
        if (!curr) {
            kfree(tokens);
            return EXT2_BAD_INO;
        }
    }

    uint32_t inode_num = curr->data.inode_num;
    kfree(tokens);
    return inode_num;
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

/// @brief Prints if inode is allocated or free
bool ext2::check_inode_status(uint32_t inode_num) {
    if(!curr_fs) {
        kprintf(LOG_WARNING, "istat: You are not in a Ext FS!\n");
        return false;
    }
    uint8_t* inode_bitmap = ext2::get_inode_bitmap(curr_fs, (inode_num - 1) / curr_fs->inodes_per_group);

    if (!inode_bitmap) {
        kprintf(LOG_ERROR, "istat: inode bitmap not provided!\n");
        kfree(inode_bitmap);
        return false;
    }

    kfree(inode_bitmap);
    // In ext filesystems, inode numbers start from 1
    return TEST_BIT(inode_bitmap, inode_num - 1);
}
