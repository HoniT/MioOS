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
#include <lib/data/string.hpp>
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
    vga::printf("%S", node.name);

    // Current dir indicator
    data::string temp = node.path;
    if(node.is_dir && !node.path.equals("/")) temp.append("/");
    if(temp == cmd::currentDir) vga::warning(" <- You are here!");
    vga::printf("\n");
}

// Gets VFS node from path
vfsNode* vfs::get_node(data::string path) {
    return &(vfs::get_tree_node(path)->data);
}
// Gets tree node from path
treeNode* vfs::get_tree_node(data::string path) {
    if(!path || path.empty()) return nullptr;
    // Spliting path to tokens
    int count;
    data::string* tokens = split_path_tokens(path, count);
    
    // Checking if path starts with root
    if(tokens[0][0] != '/') {
        vga::warning("Illegal path (%S) given to get_node!\n", path);
        
        // Free tokens before returning
        for (int i = 0; i < count; i++) tokens[i].~string();
        kfree(tokens);
        
        return nullptr;
    }
    
    // Traversing VFS tree
    treeNode* curr = vfs_tree.get_root();
    for(int i = 1; i < count; i++) {
        // Node name to find
        data::string token = tokens[i];
        
        // Getting node by comparing the names of the current node's children to the target(tokens[i])
        curr = vfs_tree.find_child_by_predicate(curr, [token](const vfsNode& node) { return node.name == token; });
        
        if (!curr) {
            vga::warning("Path component (%S) not found!\n", token);
            
            // Free tokens before returning
            for (int j = 0; j < count; j++) tokens[j].~string();
            kfree(tokens);
            
            return nullptr;
        }
    }

    // Free tokens after done
    for (int i = 0; i < count; i++) tokens[i].~string();
    kfree(tokens);

    return curr;
}

// Adds a dir/file
void vfs::add_node(data::string path, inode_t* inode) {
    if(!path || !inode) return;
    // vga::error(" add here1\n");

    // Splitting path to tokens
    int count;
    data::string* tokens = split_path_tokens(path, count);
    if(!tokens || !tokens[0].equals("/")) return; // If the path doesn't start with the root dir
    // vga::error(" add here2\n");

    // Checking if path leading to the final node exists
    treeNode* curr = vfs_tree.get_root();
    for(int i = 1; i < count - 1; i++) {
        data::string node_name_to_find = tokens[i];
        treeNode* node_to_find = vfs_tree.find_child_by_predicate(curr, [node_name_to_find](vfsNode& node){
            return node.name == node_name_to_find;
        });

        // If we can't find the next node as a child, we'll add it without its inode
        if(!node_to_find) {
            // Creating path for this node
            data::string path = curr->data.path;
            path.append(node_name_to_find);

            treeNode* new_node = vfs_tree.create({data::string(node_name_to_find), path, true, nullptr});

            vfs_tree.add_child(curr, new_node);
            curr = new_node;
        }
        else curr = node_to_find;
    }
    // vga::error(" add here3\n");

    // Adding node to the actual destination (or add the inode if it exists)
    data::string node_name_to_find = tokens[count - 1];
    treeNode* node_to_find = vfs_tree.find_child_by_predicate(curr, [node_name_to_find](vfsNode& node){
        return node.name == node_name_to_find;
    });
    // vga::error(" add here4\n");

    // If node exists and inode is not there we'll add the inode
    if(node_to_find && !node_to_find->data.inode) {
        node_to_find->data.inode = inode;
        return;
    }
    
    // If the node doesn't exist in tree at all we'll add the node
    else if(!node_to_find) {
        // vga::error(" add here5\n");
        // Creating path for this node
        data::string path = curr->data.path;
        path.append(node_name_to_find);

        vfs_tree.add_child(curr, vfs_tree.create({data::string{node_name_to_find}, path, (bool)INODE_IS_DIR(inode), inode}));
        return;
    }
}
