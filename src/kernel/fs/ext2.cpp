// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// ext2.cpp
// Sets up the Ext2 file system
// ========================================

#include <fs/ext2.hpp>
#include <fs/vfs.hpp>
#include <mm/heap.hpp>
#include <drivers/ata.hpp>
#include <drivers/vga_print.hpp>
#include <kterminal.hpp>
#include <lib/mem_util.hpp>
#include <lib/string_util.hpp>

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
    ext2::read_block(fs, block_num, (uint16_t*)buffer, 1);
    return (inode_t*)(buffer + offset_in_block);
}

// Translates Ext2 block number to LBA and reads from device using our ATA/AHCI driver
void ext2::read_block(ext2_fs_t* fs, const uint32_t block_num, uint16_t* buffer, const uint32_t blocks_to_read) {
    // Translating Ext2 blocks to LBA blocks
    uint32_t lba = (block_num * fs->block_size) / 512;
    uint32_t lba_blocks = (blocks_to_read * fs->block_size) / 512;

    pio_28::read_sector(fs->dev, lba, buffer, lba_blocks);
}

// Translates Ext2 block number to LBA and writes to device using our ATA/AHCI driver
void ext2::write_block(ext2_fs_t* fs, const uint32_t block_num, uint16_t* buffer, const uint32_t blocks_to_write) {
    // Translating Ext2 blocks to LBA blocks
    uint32_t lba = (block_num * fs->block_size) / 512;
    uint32_t lba_blocks = (blocks_to_write * fs->block_size) / 512;
    
    pio_28::write_sector(fs->dev, lba, buffer, lba_blocks);
}

// Parses for dir entries in block
void parse_directory_block(ext2_fs_t* fs, uint8_t* block, vfsNode* entries, vfsNode parent, int& last_index) {
    uint32_t offset = 0;
    // Finding all dir entries in the block
    while (offset < fs->block_size) {
        dir_ent_t* entry = (dir_ent_t*)(block + offset);
        if (entry->inode == 0) break;
        if (entry->entry_size < 8) break; // Entry too small or corrupted

        inode_t* inode = ext2::load_inode(fs, entry->inode);

        // Retrieve name
        char* name = (char*)kmalloc(entry->name_len + 1);
        memcpy(name, entry->name, entry->name_len);
        name[entry->name_len] = '\0'; // Null-terminate=

        // Build full path
        char* path = (char*)kmalloc(256);
        strcpy(path, parent.path);
        if (path[strlen(path) - 1] != '/')
            strcat(path, "/");
        strcat(path, name);

        // If it's a directory, add trailing slash
        if (INODE_IS_DIR(inode)) strcat(path, "/");

        // Adding to array
        entries[last_index++] = {name, path, INODE_IS_DIR(inode), inode};

        offset += entry->entry_size;
    }
}

// Returns a list of VFS nodes of entries in the given dir
vfsNode* ext2::read_dir(ext2_fs_t* fs, vfsNode* node, int& count) {
    if(!node || !fs || !count || !node->is_dir) return nullptr;

    int last_index = 0;
    vfsNode* entries = (vfsNode*)kmalloc(sizeof(vfsNode) * 256); // Estimate 256 entries for now

    // -- Direct blocks --
    for (int i = 0; i < 12; i++) { 
        // Getting block num
        uint32_t block_num = node->inode->direct_blk_ptr[i];
        if (block_num == 0) goto return_values; // Unused, going to return values    
        
        // Reading block
        uint8_t block[fs->block_size];
        ext2::read_block(fs, block_num, (uint16_t*)block);

        parse_directory_block(fs, block, entries, *node, last_index);
    }
    
    // -- Singly inderect blocks -- 
    if (node->inode->singly_inderect_blk_ptr != 0) {
        uint32_t block_ptrs[1024];
        ext2::read_block(fs, node->inode->singly_inderect_blk_ptr, (uint16_t*)block_ptrs);
        
        for (int i = 0; i < 1024; ++i) {
            if (block_ptrs[i] == 0) goto return_values;
            // Reading block
            uint8_t block[fs->block_size];
            ext2::read_block(fs, block_ptrs[i], (uint16_t*)block);
            parse_directory_block(fs, block, entries, *node, last_index);
        }
    }

return_values:
    count = last_index;

    // Adds to VFS tree
    for(int i = 0; i < count; i++) {
        // If the entry isn't the current or parent dir
        if(strcmp(entries[i].name, ".") != 0 && strcmp(entries[i].name, "..") != 0) vfs::add_node(entries[i].path, entries[i].inode);
    }

    return entries;
}

ext2_fs_t* curr_fs;
// Initializes the Ext2 FS for an ATA device
bool ext2::init_ext2_device(ata::device_t* dev) {
    // Allocating space for the FS metadata and superblock. Adding device 
    ext2_fs_t* ext2fs = (ext2_fs_t*)kcalloc(1, sizeof(ext2_fs_t));
    ext2fs->dev = dev;
    ext2fs->sb = (superblock_t*)kmalloc(SUPERBLOCK_SIZE);

    // Reading superblock (Located at LBA 2 and takes up 2 sectors)
    pio_28::read_sector(dev, 2, (uint16_t*)ext2fs->sb, 2);
    // Verifying superblock
    if(ext2fs->sb->ext2_magic != EXT2_MAGIC) {
        vga::error("Superblock not found for ATA device: %s!\n", dev->serial);
        return false;
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
    ext2::read_block(ext2fs, (ext2fs->block_size == 1024) ? 2 : 1, (uint16_t*)ext2fs->blk_grp_descs, ext2fs->blk_grp_desc_blocks);

    // Getting root inode
    inode_t* root_inode = load_inode(ext2fs, ROOT_INODE_NUM);
    if (INODE_IS_DIR(root_inode)) {
        // Not a directory, something went wrong
        vga::error("The root inode wasn't a directory!\n");
        return false;
    }
    // Setting root inode in VFS
    vfs::set_root(root_inode);
    // Adding any entries of root
    int count;
    vfsNode* nodes = ext2::read_dir(ext2fs, &vfs_tree.get_root()->data, count);
    vga::printf("Successfully initialized Ext2 file system on ATA device %s!\n", dev->serial);
    curr_fs = ext2fs;
    return true;
}

#pragma region Terminal Functions

// Lists dir entries
void ext2::ls(void) {
    int count;
    vfsNode* nodes = ext2::read_dir(curr_fs, vfs::get_node(cmd::currentDir), count);
    if(!nodes) return;

    // Printing dir entry names
    for(int i = 0; i < count; i++) {
        if(strcmp(nodes[i].name, "..") == 0) vga::warning(".. "); // Parent dir
        else if(strcmp(nodes[i].name, ".") != 0) vga::printf("%s ", nodes[i].name);
    }
    vga::printf("\n");

    vfs_tree.traverse(vfs_tree.get_root(), vfs::print_node);

    kfree(nodes);
}


// Changes Directory
void ext2::cd(void) {
    // Getting dir
    const char* dir = get_remaining_string(cmd::currentInput);
    if(strcmp(dir, "") == 0) {
        vga::warning("Syntax: cd <dir>\n");
        return;
    }

    // Current dir as input (".")
    if(strcmp(dir, ".") == 0) return;

    // Parent dir as input ("..")
    if(strcmp(dir, "..") == 0) {
        data::tree<vfsNode>::Node* curr_tree_node = vfs::get_tree_node(cmd::currentDir);
        if(!curr_tree_node) return;

        // If we're in root dir, there is no more parent
        if(!curr_tree_node->parent) {
            vga::printf("No parent for root!\n");
            return;
        }

        strcpy(cmd::currentDir, curr_tree_node->parent->data.path);
        return;
    }

    int count;
    vfsNode* nodes = ext2::read_dir(curr_fs, vfs::get_node(cmd::currentDir), count);
    for(int i = 0; i < count; i++) {
        // If we found a match
        if(strcmp(nodes[i].name, dir) == 0) {
            strcpy(cmd::currentDir, nodes[i].path);
            kfree(nodes);
            return;
        }
    }

    vga::warning("No directory found of name %s in %s!\n", dir, cmd::currentDir);
    kfree(nodes);
}
