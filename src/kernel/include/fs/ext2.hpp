// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef EXT2_HPP
#define EXT2_HPP

#include <stdint.h>
#include <device.hpp>

#pragma region Structures

// For superblock
#define SUPERBLOCK_SIZE 1024
#define EXT2_MAGIC 0xEF53

// For inodes
#define ROOT_INODE_NUM 2
#define EXT2_S_IFDIR 0x4000
#define INODE_IS_DIR(inode) ((inode->type_and_perm & 0xF000) == EXT2_S_IFDIR)

// Structure of Ext2 Superblock
struct superblock_t {
    // -- Base Fields --
    uint32_t inodes_num;
    uint32_t blks_num;
    uint32_t blks_reserved_superusr;
    uint32_t unalloc_blk_num;
    uint32_t unalloc_inode_num;
    uint32_t superblock_blk_num;
    uint32_t blk_size;
    uint32_t frag_size;
    uint32_t blkgroup_blk_num;
    uint32_t blkgroup_frag_num;
    uint32_t blkgroup_inode_num;
    uint32_t l_mount_time;
    uint32_t l_write_time;
    uint16_t mnt_num_since_fsck;
    uint16_t mnt_num_before_fsck;
    uint16_t ext2_magic;
    uint16_t fs_state;
    uint16_t error;
    uint16_t minor_version;
    uint32_t time_since_fsck;
    uint32_t interval_between_fsck;
    uint32_t os_id;
    uint32_t major_version;
    uint16_t uid;
    uint16_t gid;

    // -- Extended Fields -- 
    uint32_t first_nrsrvd_inode;
    uint16_t inode_size;
    uint16_t blkgrp_superblk;
    uint32_t optional_feats;
    uint32_t required_feats;
    uint32_t feats_to_mnt_ro;
    uint8_t fs_id[16];
    char vol_name[16];
    char path_vol_last_mnt[64];
    uint32_t compression_algo;
    uint8_t blks_prealct_file;
    uint8_t blks_prealct_dir;
    uint16_t unused_0;
    uint8_t jrnl_id[16];
    uint32_t jrnl_inode;
    uint32_t jrnl_dvce;
    uint32_t head_orphn_inode;
    uint8_t unused_1[788];
} __attribute__((packed));

// Structure of Ext2 Block Group Descriptor
struct blkgrp_descriptor_t {
    uint32_t blk_addr_blk_usage_bitmap;
    uint32_t blk_addr_inode_usage_bitmap;
    uint32_t inode_tbl_start_blk_addr;
    uint16_t num_unalloc_blks;
    uint16_t num_unalloc_inodes;
    uint16_t num_dirs;
    uint8_t unused[14];
} __attribute__((packed));

// Structure of Ext2 Inode
struct inode_t {
    uint16_t type_and_perm;
    uint16_t uid;
    uint32_t size_low;
    uint32_t last_access_time;
    uint32_t create_time;
    uint32_t last_mod_time;
    uint32_t delete_time;
    uint16_t gid;
    uint16_t hard_link_count;
    uint32_t disk_sect_count;
    uint32_t flags;
    uint32_t os_specific_1;
    uint32_t direct_blk_ptr[12];
    uint32_t singly_inderect_blk_ptr;
    uint32_t doubly_inderect_blk_ptr;
    uint32_t triply_inderect_blk_ptr;
    uint32_t gen_num;
    uint32_t ext_attr_blk;
    uint32_t size_high;
    uint32_t frag_blk_addr;
    uint8_t os_specific_2[12];
} __attribute__((packed));

// Structure of Ext2 Directory Entry
struct dir_ent_t {
    uint32_t inode;
    uint16_t entry_size;
    uint8_t name_len;
    uint8_t type_indicator;
    char name[];
} __attribute__((packed));

#pragma endregion

struct ext2_fs_t {
    // Device
    union {
        ata::device_t* dev;
        // TODO: Add an AHCI device to this union after implementing an AHCI driver
    };

    // File system info
    superblock_t* sb;
    blkgrp_descriptor_t* blk_grp_descs;
    uint32_t block_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint32_t total_groups;
    uint32_t blk_grp_desc_blocks;
};

struct vfsNode;

namespace ext2 {
    // Initialization functions
    bool init_ext2_device(ata::device_t* dev);

    // Read / Write functions
    void read_block(ext2_fs_t* fs, const uint32_t block_num, uint16_t* buffer, const uint32_t blocks_to_read = 1);
    void write_block(ext2_fs_t* fs, const uint32_t block_num, uint16_t* buffer, const uint32_t blocks_to_write = 1);
    // Returns a list of VFS nodes of entries in the given dir
    vfsNode* read_dir(ext2_fs_t* fs, vfsNode* node, int& count);
    
    // Loads an inode with a given inode number
    inode_t* load_inode(ext2_fs_t* fs, const uint32_t inode_num);

    // Terminal functions
    // Prints dir entries
    void ls(void);
    // Changes Directory
    void cd(void);
} // namespace ext2

#endif // EXT2_HPP
