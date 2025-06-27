// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef EXT2_HPP
#define EXT2_HPP

#include <stdint.h>

#pragma region Structures

// Structure of Ext2 Superblock
struct superblock {
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
struct blkgrp_descriptor {
    uint32_t blk_addr_blk_usage_bitmap;
    uint32_t blk_addr_inode_usage_bitmap;
    uint32_t inode_tbl_start_blk_addr;
    uint16_t num_unalloc_blks;
    uint16_t num_unalloc_inodes;
    uint16_t num_dirs;
    uint8_t unused[14];
} __attribute__((packed));

// Structure of Ext2 Inode
struct inode {
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
    uint32_t direct_blk_ptr_0;
    uint32_t direct_blk_ptr_1;
    uint32_t direct_blk_ptr_2;
    uint32_t direct_blk_ptr_3;
    uint32_t direct_blk_ptr_4;
    uint32_t direct_blk_ptr_5;
    uint32_t direct_blk_ptr_6;
    uint32_t direct_blk_ptr_7;
    uint32_t direct_blk_ptr_8;
    uint32_t direct_blk_ptr_9;
    uint32_t direct_blk_ptr_10;
    uint32_t direct_blk_ptr_11;
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
struct dir_ent {
    uint32_t inode;
    uint16_t entry_size;
    uint8_t name_len;
    uint8_t type_indicator;
    char name[];
} __attribute__((packed));

#pragma endregion

#endif // EXT2_HPP
