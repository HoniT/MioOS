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
#include <lib/data/string.hpp>
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
    if (last_index >= 256) return;  // prevent overflow

    uint32_t offset = 0;
    // Finding all dir entries in the block
    while (offset < fs->block_size) {
        dir_ent_t* entry = (dir_ent_t*)(block + offset);
        if (entry->inode == 0) break;
        if (entry->entry_size < 8) break; // Entry too small or corrupted
        
        inode_t* inode = ext2::load_inode(fs, entry->inode);
        
        // Retrieve name
        data::string name(entry->name, entry->name_len);
        if(!inode) vga::warning("No inode for %S\n", name);
        
        // Build full path
        data::string path = parent.path;
        path.append(name);
        
        // If it's a directory, add trailing slash
        if (INODE_IS_DIR(inode)) path.append("/");
        
        // Adding to array
        entries[last_index++] = {name, path, INODE_IS_DIR(inode), inode};
        
        offset += entry->entry_size;
    }
}

ext2_fs_t* curr_fs = nullptr;
// Returns a list of VFS nodes of entries in the given dir
vfsNode* ext2::read_dir(treeNode* tree_node, int& count) {
    if (!tree_node || !count) return nullptr;
    vfsNode* node = &tree_node->data;
    if (!node->is_dir) {
        vga::error("read_dir error: node is not a directory!\n");
        return nullptr;
    }
    int last_index = 0;
    vfsNode* entries = (vfsNode*)kmalloc(sizeof(vfsNode) * 256); // Estimate 256 entries for now
    // If we're not in a physical FS (In virtually added directories e.g. "/" (root)) we'll find children in the VFS tree
    if(!curr_fs) {
        treeNode* curr = tree_node->first_child;
        entries = (vfsNode*)kmalloc(sizeof(vfsNode) * 256);
        while(curr) {
            entries[last_index++] = curr->data;
            curr = curr->next_sibling;
        }
        count = last_index;
        return entries;
    }

    entries = (vfsNode*)kmalloc(sizeof(vfsNode) * 256);
    // -- Direct blocks --
    for (int i = 0; i < 12; i++) { 
        // Getting block num
        uint32_t block_num = node->inode->direct_blk_ptr[i];
        if (block_num == 0) continue; // Unused
        
        // Reading block
        uint8_t block[curr_fs->block_size];
        ext2::read_block(curr_fs, block_num, (uint16_t*)block);
        
        parse_directory_block(curr_fs, block, entries, *node, last_index);
    }
    
    // -- Singly inderect blocks -- 
    if (node->inode->singly_inderect_blk_ptr != 0) {
        uint32_t block_ptrs[1024];
        ext2::read_block(curr_fs, node->inode->singly_inderect_blk_ptr, (uint16_t*)block_ptrs);
        
        for (int i = 0; i < 1024; ++i) {
            if (block_ptrs[i] == 0) continue;
            // Reading block
            uint8_t block[curr_fs->block_size];
            ext2::read_block(curr_fs, block_ptrs[i], (uint16_t*)block);
            parse_directory_block(curr_fs, block, entries, *node, last_index);
        }
    }

    count = last_index;
    return entries;
}

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
    if (root_inode->type_and_perm & 0xF000 == 0x4000) {
        // Not a directory, something went wrong
        vga::error("The root inode wasn't a directory!\n");
        return false;
    }

    // Mounting to VFS
    vfs::mount_dev(vfs::device_names[vfs::device_name_index++], root_inode, ext2fs);

    vga::printf("Successfully initialized Ext2 file system on ATA device %s!\n", dev->serial);
    return true;
}

#pragma Terminal Functions

/// @brief Lists directory entries
void ext2::ls(void) {
    treeNode* node = vfs::get_node(cmd::currentDir); // Retreiving node from current path
    if(!node) return;
    
    int count;
    vfsNode* nodes = ext2::read_dir(node, count);
    if(!nodes) return;

    // Printing
    for(int i = 0; i < count; i++) {
        vga::printf("%S ", nodes[i].name);
    }
    vga::printf("\n");
}

/// @brief Changes directory
void ext2::cd(void) {
    // Getting directory to change to
    data::string dir = get_remaining_string(cmd::currentInput);
    if(dir.empty()) {
        vga::warning("Syntax: cd <dir>\n");
        return;
    }

    // Current dir
    if(dir.equals(".")) return;
    
    // Finding child (dir to find) from current node (VFS tree)
    treeNode* currNode = vfs::get_node(cmd::currentDir);
    // Parent dir
    if(dir.equals("..")) {
        // If we're in root
        if(!currNode->parent) {
            vga::warning("No parent for root dir!\n");
            return;
        }

        cmd::currentDir = currNode->parent->data.path;
        return;
    }
    
    treeNode* found_dir = vfs_tree.find_child_by_predicate(currNode, [dir](vfsNode node){return node.name == dir;});
    // If we found in tree
    if(found_dir) {
        cmd::currentDir = found_dir->data.path;
        // Setting FS if possible
        if(curr_fs != found_dir->data.fs) curr_fs = found_dir->data.fs;
        return;
    }

    // If we're in virtual dirs, it's pointless to check again physically
    if (!curr_fs) {
        vga::warning("Couldn't find directory \"%S\" in \"%S\"\n", dir, cmd::currentDir);
        return;
    }
    
    // If we couldnt find the dir in the virtual tree, we'll check physically
    int count;
    vfsNode* nodes = ext2::read_dir(currNode, count);
    if(!nodes) return;
    for(int i = 0; i < count; i++) {
        // If we found it here
        if(nodes[i].name == dir) {
            cmd::currentDir = nodes[i].path;
            // Setting FS if possible
            if(curr_fs != nodes[i].fs) curr_fs = nodes[i].fs;
            
            // Adding to VFS tree
            vfs::add_node(currNode, nodes[i].name, nodes[i].inode, nodes[i].fs);
            return;
        }
    }

    vga::warning("Couldn't find directory \"%S\" in \"%S\"\n", dir, cmd::currentDir);
    return;
}

#pragma endregion
