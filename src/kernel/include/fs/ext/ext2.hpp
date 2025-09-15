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
#include <lib/data/list.hpp>


// For superblock
#define SUPERBLOCK_SIZE 1024
#define EXT2_MAGIC 0xEF53

#define EXT2_BAD_INO             1  /* Bad blocks inode */
#define EXT2_ROOT_INO            2  /* Root directory */
#define EXT2_BOOT_LOADER_INO     5  /* Boot loader */
#define EXT2_UNDEL_DIR_INO       6  /* Undelete directory */
#define EXT2_FIRST_NONRESERVED_INO 11

#define EXT2_S_IFMT   0xF000  // Format mask
#define EXT2_S_IFSOCK 0xC000  // Socket
#define EXT2_S_IFLNK  0xA000  // Symbolic link
#define EXT2_S_IFREG  0x8000  // Regular file
#define EXT2_S_IFBLK  0x6000  // Block device
#define EXT2_S_IFDIR  0x4000  // Directory
#define EXT2_S_IFCHR  0x2000  // Character device
#define EXT2_S_IFIFO  0x1000  // FIFO

#define EXT2_S_IRUSR 0x0100 // Owner has read permission
#define EXT2_S_IWUSR 0x0080 // Owner has write permission
#define EXT2_S_IXUSR 0x0040 // Owner has execute permission
#define EXT2_S_IRGRP 0x0020 // Group has read permission
#define EXT2_S_IWGRP 0x0010 // Group has write permission
#define EXT2_S_IXGRP 0x0008 // Group has execute permission
#define EXT2_S_IROTH 0x0004 // Others have read permission
#define EXT2_S_IWOTH 0x0002 // Others have write permission
#define EXT2_S_IXOTH 0x0001 // Others have execute permission

#define EXT2_FT_UNKNOWN  0   // Unknown file type
#define EXT2_FT_REG_FILE 1   // Regular file
#define EXT2_FT_DIR      2   // Directory
#define EXT2_FT_CHRDEV   3   // Character device
#define EXT2_FT_BLKDEV   4   // Block device
#define EXT2_FT_FIFO     5   // FIFO (named pipe)
#define EXT2_FT_SOCK     6   // Socket
#define EXT2_FT_SYMLINK  7   // Symbolic link

#define INODE_IS_DIR(inode) ((inode->type_and_perm & EXT2_S_IFMT) == EXT2_S_IFDIR)
#define INODE_IS_FILE(inode) ((inode->type_and_perm & EXT2_S_IFMT) == EXT2_S_IFREG)
#define DEFAULT_PERMS 0755 // rwxr-xr-x

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
    // Initialization functions
    ext2_fs_t* init_ext2_device(ata::device_t* dev);
    // Finds all Ext2 File Systems
    void find_ext2_fs(void);

    data::string mode_to_string(const uint16_t mode);
    ext2_perms get_perms(const inode_t* inode, const uint32_t uid, const uint32_t gid);
    uint8_t get_inode_type(const inode_t* inode);

    // Read / Write functions
    bool read_block(ext2_fs_t* fs, const uint32_t block_num, uint8_t* buffer, const uint32_t blocks_to_read = 1);
    bool write_block(ext2_fs_t* fs, const uint32_t block_num, uint8_t* buffer, const uint32_t blocks_to_write = 1);
    // Returns a list of VFS nodes of entries in the given dir
    data::list<vfsNode> read_dir(data::tree<vfsNode>::Node* node);
    
    uint8_t* get_inode_bitmap(ext2_fs_t* fs, uint32_t group);
    void write_inode_bitmap(ext2_fs_t* fs, uint32_t group, uint8_t* bitmap);
    // Loads an inode with a given inode number
    inode_t* load_inode(ext2_fs_t* fs, const uint32_t inode_num);
    void free_inode(ext2_fs_t* fs, const uint32_t inode_num);
    // Allocates inode
    uint32_t alloc_inode(ext2_fs_t* fs);   
    // Writes inode info to disk
    void write_inode(ext2_fs_t* fs, const uint32_t inode_num, inode_t* inode);

    uint8_t* get_block_bitmap(ext2_fs_t* fs, uint32_t group);
    void write_block_bitmap(ext2_fs_t* fs, uint32_t group, uint8_t* bitmap);
    // Allocates block
    uint32_t alloc_block(ext2_fs_t* fs);
    void free_block(ext2_fs_t* fs, uint32_t block_num);
    void free_blocks(ext2_fs_t* fs, inode_t* inode);

    void clear_bitmap_bit(uint8_t* bitmap, uint32_t bit);

    // Rewrites block group descriptors of a FS
    void rewrite_bgds(ext2_fs_t* fs);
    // Rewrites superblock of a FS
    void rewrite_sb(ext2_fs_t* fs);

    // Terminal functions
    void pwd(data::list<data::string> params);
    void ls(data::list<data::string> params);
    void cd(data::list<data::string> params);
    void mkdir(data::list<data::string> params);
    void mkfile(data::list<data::string> params);
    void rm(data::list<data::string> params);
} // namespace ext2

#endif // EXT2_HPP
