// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VFS_HPP
#define VFS_HPP

#include <lib/data/tree.hpp>
#include <lib/data/string.hpp>
#include <fs/ext/ext2.hpp>

typedef data::tree<vfsNode>::Node treeNode;

// VFS node (directory / file)
struct vfsNode {
    data::string name;
    data::string path;
    bool is_dir;
    uint32_t inode_num; // Extra data (inode_number)
    inode_t* inode; // Extra data (inode)
    ext2_fs_t* fs; // Extra data (FS)
};

extern data::tree<vfsNode> vfs_tree;

namespace vfs {
    extern int device_name_index;
    extern const char* device_names[26];
    extern int ide_device_name_index;
    extern const char* ide_device_names[4];

    // Initializes the VFS
    void init(void);
    // Prints VFS tree node
    void print_node(const vfsNode& node, int depth);
    void add_node(treeNode* parent, data::string name);
    void add_node(treeNode* parent, data::string name, uint32_t inode_num, inode_t* inode, ext2_fs_t* fs);
    treeNode* get_node(const data::string path);
    void mount_dev(data::string name, inode_t* root_inode, ext2_fs_t* fs);
} // namespace vfs

#endif // VFS_HPP
