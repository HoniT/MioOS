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

int vfs::device_name_index = 0;
const char* vfs::device_names[26] = {
    "sda", "sdb", "sdc", "sdd", "sde", "sdf", "sdg", "sdh", "sdi", "sdj",
    "sdk", "sdl", "sdm", "sdn", "sdo", "sdp", "sdq", "sdr", "sds", "sdt",
    "sdu", "sdv", "sdw", "sdx", "sdy", "sdz"
};

int vfs::ide_device_name_index = 0;
const char* vfs::ide_device_names[4] = {
    "hda", "hdb", "hdc", "hdd"
};

// Prints VFS tree node
void vfs::print_node(const vfsNode& node, int depth) {
    for(int i = 0; i < depth; i++) vga::printf(" ");
    vga::printf("%S\n", node.name);
}

// Initializes the VFS
void vfs::init(void) {
    // Creating and setting root dir
    kmalloc(sizeof(ext2_fs_t)); // Reserving memory
    vfs_tree.set_root(vfs_tree.create({"/", "/", true, 0, nullptr, nullptr}));
    // Adding mount directory
    vfs_tree.add_child(vfs_tree.get_root(), vfs_tree.create({"mnt", "/mnt/", true, 0, nullptr, nullptr}));
}

/// @brief Adds a virtual node (dir, file...) to the VFS
/// @param parent Parent of the node to add
/// @param node node to add
void vfs::add_node(treeNode* parent, data::string name) {
    vfs::add_node(parent, name, 0, nullptr, nullptr);
}

/// @brief Adds a physical node (dir, file...) to the VFS
/// @param parent Parent of the node to add
/// @param node node to add
/// @param inode Inode pointing to actual FS dir entry
void vfs::add_node(treeNode* parent, data::string name, uint32_t inode_num, inode_t* inode, ext2_fs_t* fs) {
    if(name.empty() || !parent) return;
    if(!parent->data.is_dir) {
        vga::warning("Parent passed to add_node isn't a dir!\n");
        return;
    }

    // Creating path for new node
    data::string path = parent->data.path;
    path.append(name);
    path.append("/");

    // Checking if parent already has a child of node
    treeNode* child = vfs_tree.find_child_by_predicate(parent, [path](vfsNode curr){return curr.path == path;});
    if(child) {
        // Adding inode and/or fs if it's missing
        if(!child->data.inode && inode) child->data.inode = inode;
        if(!child->data.fs && fs) child->data.fs = fs; 
        return;
    }

    // Adding the node
    vfs_tree.add_child(parent, vfs_tree.create({name, path, inode ? INODE_IS_DIR(inode) : false, inode_num, inode, fs}));
}

/// @brief Gets a VFS node with a given path
/// @param path Path of node
/// @return Tree node of path
treeNode* vfs::get_node(const data::string path) {
    if(path.empty()) return nullptr;

    // Spliting path into individual tokens
    int count;
    data::string* tokens = split_path_tokens(path, count);
    if(!tokens || count == 0 || !tokens[0].equals("/")) return nullptr;

    treeNode* curr = vfs_tree.get_root();
    for(int i = 1; i < count; i++) {
        data::string name_to_find = tokens[i];
        curr = vfs_tree.find_child_by_predicate(curr, [name_to_find](vfsNode node){return node.name == name_to_find;});
        if(!curr) {
            // Freeing memory
            name_to_find.~string();
            kfree(tokens);
        }
    }

    return curr;
}

/// @brief Mounts a device to VFS
/// @param root_inode Inode #2 of Ext2 device
/// @param fs File System of device
void vfs::mount_dev(data::string name, inode_t* root_inode, ext2_fs_t* fs) {
    // Getting mount dir
    treeNode* mntNode = vfs::get_node("/mnt/");
    if(!mntNode) return;
    // Adding node
    vfs::add_node(mntNode, name, ROOT_INODE_NUM, root_inode, fs);
}