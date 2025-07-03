// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VFS_HPP
#define VFS_HPP

#include <lib/data/tree.hpp>
#include <fs/ext2.hpp>

// VFS node (directory / file)
struct vfsNode {
    const char* name;
    bool is_dir;
    void* extra; // Extra data
};

extern data::tree<vfsNode> vfs_tree;

namespace vfs {
    // Sets up root VFS node from Inode #2
    void set_root(inode_t* inode);
} // namespace vfs

#endif // VFS_HPP
