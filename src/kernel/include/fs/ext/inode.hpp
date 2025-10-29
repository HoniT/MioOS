// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef EXT2_INODE_HPP
#define EXT2_INODE_HPP

#include <stdint.h>
#include <fs/ext/ext2.hpp>

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

#define EXT2_BAD_INO             1  /* Bad blocks inode */
#define EXT2_ROOT_INO            2  /* Root directory */
#define EXT2_BOOT_LOADER_INO     5  /* Boot loader */
#define EXT2_UNDEL_DIR_INO       6  /* Undelete directory */
#define EXT2_FIRST_NONRESERVED_INO 11

#define INODE_IS_DIR(inode) ((inode->type_and_perm & EXT2_S_IFMT) == EXT2_S_IFDIR)
#define INODE_IS_FILE(inode) ((inode->type_and_perm & EXT2_S_IFMT) == EXT2_S_IFREG)

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

namespace ext2 {
    // Inode specific functions

    // Helpers
    uint8_t get_inode_type(const inode_t* inode);
    uint32_t find_inode(ext2_fs_t* fs, data::string path);
    bool check_inode_status(uint32_t inode_num);

    uint8_t* get_inode_bitmap(ext2_fs_t* fs, uint32_t group);
    void write_inode_bitmap(ext2_fs_t* fs, uint32_t group, uint8_t* bitmap);
    // Loads an inode with a given inode number
    inode_t* load_inode(ext2_fs_t* fs, const uint32_t inode_num);
    void free_inode(ext2_fs_t* fs, const uint32_t inode_num);
    // Allocates inode
    uint32_t alloc_inode(ext2_fs_t* fs);   
    // Writes inode info to disk
    void write_inode(ext2_fs_t* fs, const uint32_t inode_num, inode_t* inode);
} // namespace ext2

#endif // EXT2_INODE_HPP
