// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VFS_HPP
#define VFS_HPP

#include <lib/data/tree.hpp>
#include <fs/ext2.hpp>

typedef data::tree<vfsNode>::Node treeNode;

// VFS node (directory / file)
struct vfsNode {
    const char* name;
    const char* path;
    bool is_dir;
    inode_t* inode; // Extra data (inode)
};

extern data::tree<vfsNode> vfs_tree;

namespace vfs {
    // Sets up root VFS node from Inode #2
    void set_root(inode_t* inode);
    // Prints VFS tree node
    void print_node(const vfsNode& node, int depth);
    // Gets VFS node from path
    vfsNode* get_node(const char* path);
    // Gets tree node from path
    treeNode* get_tree_node(const char* path);
    // Adds a dir/file
    void add_node(const char* path, inode_t* inode);
} // namespace vfs

#endif // VFS_HPP
