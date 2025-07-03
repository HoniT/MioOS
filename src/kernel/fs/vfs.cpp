// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// vfs.cpp
// Sets up the virtual file system
// ========================================

#include <fs/vfs.hpp>
#include <fs/ext2.hpp>
#include <mm/heap.hpp>
#include <drivers/vga_print.hpp>
#include <lib/data/tree.hpp>

data::tree<vfsNode> vfs_tree;

// Helper function for printing VFS node
void print_node(const vfsNode& node, int depth) {
    for (int i = 0; i < depth; ++i) vga::printf(" ");
    vga::printf(node.name);
    vga::printf("\n");
}

void vfs::set_root(inode_t* inode) {
    // Creating node for root
    auto* root = vfs_tree.create({"/", true, (void*)inode});
    vfs_tree.set_root(root);
}
