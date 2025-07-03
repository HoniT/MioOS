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

// Loads an inode with a given inode number
inode_t* load_inode(ext2_fs_t* fs, const uint32_t inode_num) {
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
    if ((root_inode->type_and_perm & 0xF000) != EXT2_S_IFDIR) {
        // Not a directory, something went wrong
        vga::error("The root inode wasn't a directory!\n");
        return false;
    }
    // Setting root inode in VFS
    vfs::set_root(root_inode);
    vga::printf("Successfully initialized Ext2 file system on ATA device %s!\n", dev->serial);
    return true;
}
