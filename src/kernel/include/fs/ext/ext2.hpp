// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef EXT2_HPP
#define EXT2_HPP

#include <stdint.h>
#include <device.hpp>
#include <lib/data/tree.hpp>
#include <lib/data/string.hpp>
#include <lib/data/large_string.hpp>
#include <lib/data/list.hpp>

// For superblock
#define SUPERBLOCK_SIZE 1024
#define EXT2_MAGIC 0xEF53

#define DEFAULT_PERMS 0755 // rwxr-xr-x
#define RESTRICTED_PERMS 0700 // rwx------

#define TEST_BIT(bitmap, bit)  ((bitmap[(bit) / 8] >> ((bit) % 8)) & 1)
#define SET_BIT(bitmap, bit)   (bitmap[(bit) / 8] |= (1 << ((bit) % 8)))
#define CLEAR_BIT(bitmap, bit) (bitmap[(bit) / 8] &= ~(1 << ((bit) % 8)))

#pragma region Structures
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

struct inode_t;

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

    // MBR partition start
    uint32_t partition_start = 0;

    // File system info
    superblock_t* sb;
    blkgrp_descriptor_t* blk_grp_descs;
    uint32_t block_size;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint32_t total_groups;
    uint32_t blk_grp_desc_blocks;
};

struct ext2_perms {
    bool read;
    bool write;
    bool execute;
};

struct vfsNode;

namespace ext2 {
    extern ext2_fs_t* curr_fs;

    // Initialization functions
    ext2_fs_t* init_ext2_device(ata::device_t* dev, bool sysdisk_check);
    // Finds all Ext2 File Systems
    void find_ext2_fs(void);
    void find_other_ext2_fs(ata::device_t* dev);

    data::string mode_to_string(const uint16_t mode);
    ext2_perms get_perms(const inode_t* inode, const uint32_t uid, const uint32_t gid);

    // Returns a list of VFS nodes of entries in the given dir
    data::list<vfsNode> read_dir(data::tree<vfsNode>::Node* node);
    bool change_dir(data::string dir);

    void clear_bitmap_bit(uint8_t* bitmap, uint32_t bit);

    // Rewrites block group descriptors of a FS
    void rewrite_bgds(ext2_fs_t* fs);
    // Rewrites superblock of a FS
    void rewrite_sb(ext2_fs_t* fs);

    void remove_entry(data::tree<vfsNode>::Node* node_to_remove);

    data::large_string get_file_contents(data::string path);
    bool write_file_content(data::string path, const data::string input, bool overwrite = true);

    void make_dir(data::string dir, vfsNode parent, data::tree<vfsNode>::Node* node, uint16_t perms);
    void make_file(data::string file, vfsNode parent, data::tree<vfsNode>::Node* node, uint16_t perms);
} // namespace ext2

#endif // EXT2_HPP
