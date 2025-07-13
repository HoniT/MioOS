// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// vfs.cpp
// Sets up the virtual file system
// ========================================

#include <fs/vfs.hpp>
#include <fs/ext2.hpp>
#include <drivers/vga_print.hpp>
#include <kterminal.hpp>
#include <lib/data/tree.hpp>
#include <lib/path_util.hpp>
#include <lib/string_util.hpp>
#include <lib/mem_util.hpp>

data::tree<vfsNode> vfs_tree;

// Sets up root VFS node from Inode #2
void vfs::set_root(inode_t* inode) {
    // Creating node for root
    auto* root = vfs_tree.create({"/", "/", true, inode});
    vfs_tree.set_root(root);
}

// Prints VFS tree node
void vfs::print_node(const vfsNode& node, int depth) {
    for(int i = 0; i < depth; i++) vga::printf(" ");
    vga::printf(node.name);

    // Current dir indicator
    if(strcmp(cmd::currentDir, node.path) == 0) vga::warning(" <- You are here!");
    vga::printf("\n");
}

// Gets VFS node from path
vfsNode* vfs::get_node(const char* path) {
    return &(vfs::get_tree_node(path)->data);
}
// Gets tree node from path
treeNode* vfs::get_tree_node(const char* path) {
    if(!path) return nullptr;

    // Spliting path to tokens
    int count;
    char** tokens = split_path_tokens(path, count);

    // Checking if path starts with root
    if(tokens[0][0] != '/') {
        vga::warning("Illegal path (%s) given to get_node!\n", path);

        // Free tokens before returning
        for (int i = 0; i < count; i++) kfree(tokens[i]);
        kfree(tokens);

        return nullptr;
    }

    // Traversing VFS tree
    treeNode* curr = vfs_tree.get_root();
    for(int i = 1; i < count; i++) {
        // Node name to find
        const char* token = tokens[i];

        // Getting node by comparing the names of the current node's children to the target(tokens[i])
        curr = vfs_tree.find_child_by_predicate(curr, [token](const vfsNode& node) { return strcmp(node.name, token) == 0; });

        if (!curr) {
            vga::warning("Path component (%s) not found!\n", token);

            // Free tokens before returning
            for (int j = 0; j < count; j++) kfree(tokens[j]);
            kfree(tokens);

            return nullptr;
        }
    }

    // Free tokens after done
    for (int i = 0; i < count; i++) kfree(tokens[i]);
    kfree(tokens);

    return curr;
}

// Adds a dir/file
void vfs::add_node(const char* path, inode_t* inode) {
    if(!path || !inode) return;

    // Splitting path to tokens
    int count;
    char** tokens = split_path_tokens(path, count);
    if(!tokens || strcmp(tokens[0], "/") != 0) return; // If the path doesn't start with the root dir

    // Checking if path leading to the final node exists
    treeNode* curr = vfs_tree.get_root();
    for(int i = 1; i < count - 1; i++) {
        char* node_name_to_find = tokens[i];
        treeNode* node_to_find = vfs_tree.find_child_by_predicate(curr, [node_name_to_find](vfsNode& node){
            return strcmp(node.name, node_name_to_find) == 0;
        });

        // If we can't find the next node as a child, we'll add it without its inode
        if(!node_to_find) {
            // Creating path for this node
            char* path_buf = (char*)kmalloc(256);
            strcpy(path_buf, curr->data.path);
            size_t base_len = strlen(path_buf);
            memcpy(path_buf + base_len, node_name_to_find, strlen(node_name_to_find));
            path_buf[base_len + strlen(node_name_to_find)] = '\0'; // Proper null-terminate

            // Adding the leading nodes as a directory without linking the actual inode
            char* name_copy = (char*)kmalloc(strlen(node_name_to_find) + 1);
            strcpy(name_copy, node_name_to_find);
            char* path_copy = (char*)kmalloc(strlen(path_buf) + 1);
            strcpy(path_copy, path_buf);
            treeNode* new_node = vfs_tree.create({name_copy, path_copy, true, nullptr});

            vfs_tree.add_child(curr, new_node);
            curr = new_node;
        }
        else curr = node_to_find;
    }

    // Adding node to the actual destination (or add the inode if it exists)
    char* node_name_to_find = tokens[count - 1];
    treeNode* node_to_find = vfs_tree.find_child_by_predicate(curr, [node_name_to_find](vfsNode& node){
        return strcmp(node.name, node_name_to_find) == 0;
    });

    // If node exists and inode is not there we'll add the inode
    if(node_to_find && !node_to_find->data.inode) {
        node_to_find->data.inode = inode;
        return;
    }
    // If the node doesn't exist in tree at all we'll add the node
    else if(!node_to_find) {
        // Creating path for this node
        char* path_buf = (char*)kmalloc(256);
        strcpy(path_buf, curr->data.path);
        size_t base_len = strlen(path_buf);
        memcpy(path_buf + base_len, node_name_to_find, strlen(node_name_to_find));
        path_buf[base_len + strlen(node_name_to_find)] = '\0';

        char* name_copy = (char*)kmalloc(strlen(node_name_to_find) + 1);
        strcpy(name_copy, node_name_to_find);
        char* path_copy = (char*)kmalloc(strlen(path_buf) + 1);
        strcpy(path_copy, path_buf);
        vfs_tree.add_child(curr, vfs_tree.create({name_copy, path_copy, INODE_IS_DIR(inode), inode}));
        return;
    }
}
