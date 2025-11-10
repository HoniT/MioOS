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
#include <fs/ext/inode.hpp>

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
    extern data::string currentDir;
    extern uint32_t currUid;
    extern uint32_t currGid;
    extern int device_name_index;
    extern const char* device_names[26];
    extern int ide_device_name_index;
    extern const char* ide_device_names[4];

    // Initializes the VFS
    treeNode* init(ext2_fs_t* fs);
    treeNode* init(void);
    // Prints VFS tree node
    void print_node(const treeNode* node, int depth);
    void print_tree();
    vfsNode add_node(treeNode* parent, vfsNode node);
    vfsNode add_node(treeNode* parent, data::string name, uint32_t inode_num, inode_t* inode, ext2_fs_t* fs);
    inode_t* find_inode(ext2_fs_t* fs, data::string path);
    treeNode* get_node(const data::string path);
    void mount_dev(data::string name, inode_t* root_inode, ext2_fs_t* fs);
} // namespace vfs

#endif // VFS_HPP
