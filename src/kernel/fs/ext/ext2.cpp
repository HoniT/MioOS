// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// ext2.cpp
// Sets up the Ext2 file system
// ========================================

#include <fs/ext/ext2.hpp>
#include <fs/ext/vfs.hpp>
#include <mm/heap.hpp>
#include <drivers/ata.hpp>
#include <drivers/vga_print.hpp>
#include <interrupts/kernel_panic.hpp>
#include <rtc.hpp>
#include <kterminal.hpp>
#include <lib/mem_util.hpp>
#include <lib/data/string.hpp>
#include <lib/string_util.hpp>
#include <lib/path_util.hpp>

#pragma region Read & Write

// Loads an inode with a given inode number
inode_t* ext2::load_inode(ext2_fs_t* fs, const uint32_t inode_num) {
    // Determining which Block Group contains an Inode
    uint32_t group = (inode_num - 1) / fs->sb->blkgroup_inode_num;
    // Finding an inode inside of the Block Group
    uint32_t index = (inode_num - 1) % fs->sb->blkgroup_inode_num;
    // Retreiving BGD
    blkgrp_descriptor_t* desc = &fs->blk_grp_descs[group];

    uint32_t inode_size = fs->sb->inode_size;
    uint32_t inode_table_block = desc->inode_tbl_start_blk_addr;
    uint32_t offset = index * inode_size;

    uint32_t block_num = inode_table_block + (offset / fs->block_size);
    uint32_t offset_in_block = offset % fs->block_size;

    uint8_t* buffer = (uint8_t*)kmalloc(fs->block_size);
    ext2::read_block(fs, block_num, buffer);
    return (inode_t*)(buffer + offset_in_block);
}

/// @brief Allocates a new inode
/// @param fs File system on where to allocate
/// @return Allocated inode index
uint32_t ext2::alloc_inode(ext2_fs_t* fs) {
    // Iterating through all of the block groups
    for (uint32_t group = 0; group < fs->total_groups; group++) {
        // If we don't have any unallocated inodes in this block group we'll go to the next one
        if (!fs->blk_grp_descs[group].num_unalloc_inodes) continue;

        // Allocate temporary buffer to hold the inode bitmap
        uint8_t* buf = (uint8_t*)kcalloc(1, fs->block_size);
        if (!buf) {
            kernel_panic("alloc_inode: kcalloc failed!");
        }

        // Retrieving inode bitmap
        uint32_t i_bitmap = fs->blk_grp_descs[group].blk_addr_inode_usage_bitmap;
        ext2::read_block(fs, i_bitmap, buf);

        // Iterating through bytes of the bitmap
        for (uint32_t byte = 0; byte < (fs->inodes_per_group + 7) / 8; byte++) {
            // If all of the inodes in this byte are allocated/used
            if (buf[byte] == 0xFF) continue;

            // Iterating through bits of the current byte
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t mask = 1 << bit;

                // If we found a free inode
                if ((buf[byte] & mask) == 0) {
                    // Calculating inode index
                    uint32_t ino = group * fs->inodes_per_group + byte * 8 + bit;

                    // Skip reserved inodes (1–10 in ext2) and lost+found (first free inode)
                    if (ino <= EXT2_FIRST_NONRESERVED_INO) continue;

                    // Marking as allocated/used
                    buf[byte] |= mask;
                    
                    // Rewriting bitmap with new value
                    ext2::write_block(fs, i_bitmap, buf);
                    
                    // Changing and rewriting metadata
                    fs->blk_grp_descs[group].num_unalloc_inodes--;
                    fs->sb->unalloc_inode_num--;
                    ext2::rewrite_bgds(fs);
                    ext2::rewrite_sb(fs);

                    // Freeing temporary buffer
                    kfree(buf);

                    // Returning allocated inode index
                    return ino;
                }
            }
        }

        // Free buffer if no free inode found in this group
        kfree(buf);
    }

    // No unallocated inode found in all of the block groups
    kernel_panic("No more inodes to allocate!");
    return (uint32_t)-1;
}


// Writes inode info to disk
void ext2::write_inode(ext2_fs_t* fs, const uint32_t inode_num, inode_t* inode) {
    // Inode index is 1-based in Ext2
    uint32_t index = inode_num - 1;
    uint32_t group = index / fs->inodes_per_group;
    uint32_t index_in_group = index % fs->inodes_per_group;

    uint32_t inode_table_block = fs->blk_grp_descs[group].inode_tbl_start_blk_addr;

    // Use on-disk inode size, not sizeof(inode_t)
    uint32_t inode_size = fs->sb->inode_size;
    uint32_t offset = index_in_group * inode_size;

    uint32_t block_offset = offset / fs->block_size;
    uint32_t byte_offset  = offset % fs->block_size;

    uint32_t block_num = inode_table_block + block_offset;

    // Reading the block where the inode is
    uint8_t* buf = (uint8_t*)kcalloc(1, fs->block_size);
    if (!buf) {
        return; // allocation failure
    }

    ext2::read_block(fs, block_num, buf);

    // Copying given info to buffer
    // Copy only inode_size (truncate or pad relative to sizeof(inode_t))
    uint32_t copy_size = (sizeof(inode_t) < inode_size) ? sizeof(inode_t) : inode_size;
    memcpy(buf + byte_offset, inode, copy_size);

    // Rewriting block
    ext2::write_block(fs, block_num, buf);

    kfree(buf);
    return; // success
}


/// @brief Allocates a new block
/// @param fs File system on where to allocate
/// @return Allocated block index
uint32_t ext2::alloc_block(ext2_fs_t* fs) {
    // Itterating through all of the block groups
    for (uint32_t group = 0; group < fs->total_groups; group++) {
        // If we don't have any unallocated blocks in this block group we'll go to the next one
        if (!fs->blk_grp_descs[group].num_unalloc_blks) continue;

        // Allocating temporary buffer to hold the block bitmap
        uint8_t* buf = (uint8_t*)kcalloc(1, fs->block_size);

        // Retrieving block bitmap
        uint32_t bitmap = fs->blk_grp_descs[group].blk_addr_blk_usage_bitmap;
        ext2::read_block(fs, bitmap, buf);

        // Itterating through bytes of the bitmap
        for (uint32_t byte = 0; byte < fs->block_size; byte++) {
            // If all of the blocks in this byte are allocated/used
            if (buf[byte] == 0xFF) continue;

            // Itterating through bits of the current byte
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t mask = 1 << bit;

                // If we found a free block
                if ((buf[byte] & mask) == 0) {
                    // Marking as allocated/used
                    buf[byte] |= mask;

                    // Rewriting bitmap with new value
                    ext2::write_block(fs, bitmap, buf);

                    // Changing and rewriting metadata
                    fs->blk_grp_descs[group].num_unalloc_blks--;
                    fs->sb->unalloc_blk_num--;
                    ext2::rewrite_bgds(fs);
                    ext2::rewrite_sb(fs);

                    // Freeing temporary buffer
                    kfree(buf);

                    // Calculating and returning block index
                    return group * fs->blocks_per_group + byte * 8 + bit;
                }
            }
        }

        // Free buffer if no free block found in this group
        kfree(buf);
    }

    // No unallocated block found in all of the block groups
    kernel_panic("No more blocks to allocate!");
    return (uint32_t)-1;
}


// Rewrites block group descriptors of a FS
void ext2::rewrite_bgds(ext2_fs_t* fs) {
    uint32_t bgd_start_block = (fs->block_size == 1024) ? 2 : 1;

    uint8_t* base = (uint8_t*)fs->blk_grp_descs;
    for(uint32_t i = 0; i < fs->blk_grp_desc_blocks; i++) 
        ext2::write_block(fs, bgd_start_block + i, base + i * fs->block_size);
}

// Rewrites superblock of a FS
void ext2::rewrite_sb(ext2_fs_t* fs) {
    if(!fs) return;

    uint8_t* buf = (uint8_t*)kcalloc(1, SUPERBLOCK_SIZE);

    // Read existing block first
    pio_28::read_sector(fs->dev, 2, reinterpret_cast<uint16_t*>(buf), 2);

    // Copy updated superblock into the correct offset
    // Goes at offset 0 in block #1
    memcpy(buf, &fs->sb, SUPERBLOCK_SIZE);

    // Write back
    pio_28::read_sector(fs->dev, 2, reinterpret_cast<uint16_t*>(buf), 2);

    kfree(buf);
}


bool ext2::read_block(ext2_fs_t* fs, const uint32_t block_num, uint8_t* buffer, const uint32_t blocks_to_read) {
    // Translating Ext2 blocks to LBA blocks
    uint32_t lba = (block_num * fs->block_size) / 512;
    uint32_t lba_blocks = (blocks_to_read * fs->block_size) / 512;

    if (fs->dev) {
        // ATA driver expects words, so cast here only
        pio_28::read_sector(fs->dev, lba, reinterpret_cast<uint16_t*>(buffer), lba_blocks);
    }
    else {
        vga::error("Ext2 FS doesn't have a device!\n");
        return false;
    }
    return true;
}

bool ext2::write_block(ext2_fs_t* fs, const uint32_t block_num, uint8_t* buffer, const uint32_t blocks_to_write) {
    uint32_t lba = (block_num * fs->block_size) / 512;
    uint32_t lba_blocks = (blocks_to_write * fs->block_size) / 512;

    if (fs->dev) {
        pio_28::write_sector(fs->dev, lba, reinterpret_cast<uint16_t*>(buffer), lba_blocks);
    }
    else {
        vga::error("Ext2 FS doesn't have a device!\n");
        return false;
    }
    return true;
}


ext2_fs_t* curr_fs = nullptr;
// Parses for dir entries in block
void parse_directory_block(ext2_fs_t* fs, uint8_t* block, vfsNode* entries, treeNode* parentNode, vfsNode parent, int& last_index) {
    if (last_index >= 256) return;  // Prevent overflow

    uint32_t offset = 0;
    // Finding all dir entries in the block
    while (offset < fs->block_size) {
        dir_ent_t* entry = (dir_ent_t*)(block + offset);
        if (entry->inode == 0) break;
        if (entry->entry_size < 8) break; // Entry too small or corrupted
        
        inode_t* inode = ext2::load_inode(fs, entry->inode);
        
        // Retrieve name
        data::string name(entry->name, entry->name_len);
        if(!inode) vga::warning("No inode for %S\n", name);
        
        // Build full path
        data::string path = parent.path;
        path.append(name);
        
        // If it's a directory, add trailing slash
        if (INODE_IS_DIR(inode)) path.append("/");
        
        // Adding to array
        vfsNode node = vfs_tree.create({name, path, INODE_IS_DIR(inode), entry->inode, inode, curr_fs})->data;
        // Checking if the entry is in the VFS tree
        if(!name.equals(".") && !name.equals("..") && !vfs_tree.find_child_by_predicate(parentNode, [node](vfsNode vfsNode){ return vfsNode.name == node.name; })) 
            vfs::add_node(parentNode, node);
        entries[last_index++] = node;
        
        offset += entry->entry_size;
    }
}

// Returns a list of VFS nodes of entries in the given dir
vfsNode* ext2::read_dir(treeNode* tree_node, int& count) {
    if (!tree_node || !count) return nullptr;
    vfsNode* node = &tree_node->data;
    if (!node->is_dir) {
        vga::error("read_dir error: node is not a directory!\n");
        return nullptr;
    }
    int last_index = 0;
    vfsNode* entries = (vfsNode*)kmalloc(sizeof(vfsNode) * 256); // Estimate 256 entries for now
    // If we're not in a physical FS (In virtually added directories e.g. "/" (root)) we'll find children in the VFS tree
    if(!curr_fs) {
        treeNode* curr = tree_node->first_child;
        entries = (vfsNode*)kmalloc(sizeof(vfsNode) * 256);
        while(curr) {
            entries[last_index++] = curr->data;
            curr = curr->next_sibling;
        }
        count = last_index;
        return entries;
    }

    auto read_and_parse = [&](uint32_t block_num) {
        if (block_num == 0) return;
        uint8_t block[curr_fs->block_size];
        ext2::read_block(curr_fs, block_num, block);
        parse_directory_block(curr_fs, block, entries, tree_node, *node, last_index);
    };

    entries = (vfsNode*)kmalloc(sizeof(vfsNode) * 256);

    // ---------------------
    // Handle Direct Blocks
    // ---------------------
    for (int i = 0; i < 12; i++) {
        read_and_parse(node->inode->direct_blk_ptr[i]);
    }

    // -----------------------------------
    // Handle Singly Indirect Block
    // -----------------------------------
    if (node->inode->singly_inderect_blk_ptr != 0) {
        uint32_t* ptrs = (uint32_t*)kmalloc(curr_fs->block_size);
        ext2::read_block(curr_fs, node->inode->singly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(ptrs));

        for (int i = 0; i < curr_fs->block_size / 4; ++i) {
            read_and_parse(ptrs[i]);
        }

        kfree(ptrs); // Free allocated block pointer list
    }

    // ------------------------------------
    // Handle Doubly Indirect Block
    // ------------------------------------
    if (node->inode->doubly_inderect_blk_ptr != 0) {
        uint32_t* dptrs = (uint32_t*)kmalloc(curr_fs->block_size);
        ext2::read_block(curr_fs, node->inode->doubly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(dptrs));

        for (int i = 0; i < curr_fs->block_size / 4; ++i) {
            if (dptrs[i] == 0) continue;

            // Read singly indirect block
            uint32_t* sptrs = (uint32_t*)kmalloc(curr_fs->block_size);
            ext2::read_block(curr_fs, dptrs[i], reinterpret_cast<uint8_t*>(sptrs));

            for (int j = 0; j < curr_fs->block_size / 4; ++j) {
                read_and_parse(sptrs[j]);
            }

            kfree(sptrs);
        }

        kfree(dptrs);
    }

    // -------------------------------------
    // Handle Triply Indirect Block
    // -------------------------------------
    if (node->inode->triply_inderect_blk_ptr != 0) {
        uint32_t* tptrs = (uint32_t*)kmalloc(curr_fs->block_size);
        ext2::read_block(curr_fs, node->inode->triply_inderect_blk_ptr, reinterpret_cast<uint8_t*>(tptrs));

        for (int i = 0; i < curr_fs->block_size / 4; ++i) {
            if (tptrs[i] == 0) continue;

            // Read doubly indirect block
            uint32_t* dptrs = (uint32_t*)kmalloc(curr_fs->block_size);
            ext2::read_block(curr_fs, tptrs[i], reinterpret_cast<uint8_t*>(dptrs));

            for (int j = 0; j < curr_fs->block_size / 4; ++j) {
                if (dptrs[j] == 0) continue;

                // Read singly indirect block
                uint32_t* sptrs = (uint32_t*)kmalloc(curr_fs->block_size);
                ext2::read_block(curr_fs, dptrs[j], reinterpret_cast<uint8_t*>(sptrs));

                for (int k = 0; k < curr_fs->block_size / 4; ++k) {
                    read_and_parse(sptrs[k]);
                }

                kfree(sptrs);
            }

            kfree(dptrs);
        }

        kfree(tptrs);
    }

    count = last_index;
    return entries;
}

#pragma endregion

#pragma region Init

// Initializes the Ext2 FS for an ATA device
bool ext2::init_ext2_device(ata::device_t* dev) {
    // Allocating space for the FS metadata and superblock. Adding device 
    ext2_fs_t* ext2fs = (ext2_fs_t*)kcalloc(1, sizeof(ext2_fs_t));
    ext2fs->dev = dev;
    ext2fs->sb = (superblock_t*)kmalloc(SUPERBLOCK_SIZE);
    
    // Reading superblock (Located at LBA 2 and takes up 2 sectors)
    pio_28::read_sector(dev, 2, (uint16_t*)ext2fs->sb, 2);
    // Verifying superblock
    if(ext2fs->sb->ext2_magic != EXT2_MAGIC) {
        // Mounting to VFS
        vfs::mount_dev(vfs::ide_device_names[vfs::ide_device_name_index++], nullptr, nullptr);
        return false;
    }
    // Saving information about device
    ext2fs->block_size = 1024 << ext2fs->sb->blk_size;
    ext2fs->blocks_per_group = ext2fs->sb->blkgroup_blk_num;
    ext2fs->inodes_per_group = ext2fs->sb->blkgroup_inode_num;

    // Total number of block groups (rounded up)
    ext2fs->total_groups = 
        (ext2fs->sb->blks_num + ext2fs->blocks_per_group - 1) / ext2fs->blocks_per_group;

    // Number of blocks needed for the Block Group Descriptor Table (rounded up)
    ext2fs->blk_grp_desc_blocks = 
        (ext2fs->total_groups * sizeof(blkgrp_descriptor_t) + ext2fs->block_size - 1) / ext2fs->block_size;

    // Allocating and reading BGD
    ext2fs->blk_grp_descs = (blkgrp_descriptor_t*)kmalloc(ext2fs->blk_grp_desc_blocks * ext2fs->block_size);
    ext2::read_block(ext2fs, (ext2fs->block_size == 1024) ? 2 : 1, reinterpret_cast<uint8_t*>(ext2fs->blk_grp_descs), ext2fs->blk_grp_desc_blocks);

    // Getting root inode
    inode_t* root_inode = load_inode(ext2fs, EXT2_ROOT_INO);
    if (root_inode->type_and_perm & 0xF000 == 0x4000) {
        // Not a directory, something went wrong
        vga::error("The root inode wasn't a directory for ATA device %S (Serial: %S)!\n", dev->drive, dev->serial);
        // Mounting to VFS
        vfs::mount_dev(vfs::ide_device_names[vfs::ide_device_name_index++], nullptr, nullptr);
        return false;
    }

    // Mounting to VFS
    vfs::mount_dev(vfs::ide_device_names[vfs::ide_device_name_index++], root_inode, ext2fs);

    return true;
}

// Finds all Ext2 File Systems
void ext2::find_ext2_fs(void) {
    // Initializing Ext2 on ATA devices
    for(int i = 0; i < sizeof(ata_devices); i++)
        if(strcmp(ata_devices[i].serial, "") != 0)
            ext2::init_ext2_device(&ata_devices[i]);

    // TODO: Find on AHCI when implemented
}

#pragma endregion

#pragma region Helper Functions

static inline uint16_t align4_u16(uint16_t x) { return (uint16_t)((x + 3u) & ~3u); }
static inline uint16_t dirent_rec_len(uint8_t name_len) { return (uint16_t)(8u + align4_u16(name_len)); }
static inline void zero_block(ext2_fs_t* fs, uint32_t blk, uint32_t block_size){
    uint8_t* z = (uint8_t*)kcalloc(1, block_size);
    ext2::write_block(fs, blk, z);
    kfree(z);
}

/// @brief Transforms the type and permissions field of an inode into a readable permission string
/// @param mode Type and permissions field of an inode
/// @return Readable string permission
data::string ext2::mode_to_string(const uint16_t mode) {
    data::string str;

    // File type
    switch ((mode & EXT2_S_IFMT)) {
        case EXT2_S_IFREG: str.append("-"); break;
        case EXT2_S_IFDIR: str.append("d"); break;
        case EXT2_S_IFLNK: str.append("l"); break;
        case EXT2_S_IFCHR: str.append("c"); break;
        case EXT2_S_IFBLK: str.append("b"); break;
        case EXT2_S_IFIFO: str.append("p"); break;
        case EXT2_S_IFSOCK: str.append("s"); break;
        default:           str.append("?"); break;
    }

    // Owner
    str.append((mode & EXT2_S_IRUSR) ? "r" : "-");
    str.append((mode & EXT2_S_IWUSR) ? "w" : "-");
    str.append((mode & EXT2_S_IXUSR) ? "x" : "-");

    // Group
    str.append((mode & EXT2_S_IRGRP) ? "r" : "-");
    str.append((mode & EXT2_S_IWGRP) ? "w" : "-");
    str.append((mode & EXT2_S_IXGRP) ? "x" : "-");

    // Others
    str.append((mode & EXT2_S_IROTH) ? "r" : "-");
    str.append((mode & EXT2_S_IWOTH) ? "w" : "-");
    str.append((mode & EXT2_S_IXOTH) ? "x" : "-");

    return str;
}

#pragma endregion

/// @brief cd (Change directory) logic
/// @param dir Dir to change to
/// @return If we could change to the dir
bool change_dir(data::string dir) {
    // Current dir, no need to change anything
    if(dir.equals(".")) return true;
    
    // Finding child (dir to find) from current node (VFS tree)
    treeNode* currNode = vfs::get_node(vfs::currentDir);
    // Parent dir
    if(dir.equals("..")) {
        // If we're in root
        if(!currNode->parent) {
            vga::warning("No parent for root dir!\n");
            return false;
        }

        // Changing FS if possible
        if(curr_fs != currNode->parent->data.fs) curr_fs = currNode->parent->data.fs;

        vfs::currentDir = currNode->parent->data.path;
        return true;
    }
    
    treeNode* found_dir = vfs_tree.find_child_by_predicate(currNode, [dir](vfsNode node){return node.name == dir;});
    // If we found in tree
    if(found_dir && found_dir->data.is_dir) {
        vfs::currentDir = found_dir->data.path;
        // Changing FS if possible
        if(curr_fs != found_dir->data.fs) curr_fs = found_dir->data.fs;
        return true;
    }

    // If we're in virtual dirs, it's pointless to check again physically
    if (!curr_fs) {
        vga::warning("Couldn't find directory \"%S\" in \"%S\"\n", dir, vfs::currentDir);
        return false;
    }
    
    // If we couldnt find the dir in the virtual tree, we'll check physically
    int count;
    vfsNode* nodes = ext2::read_dir(currNode, count);
    if(!nodes) return false;
    for(int i = 0; i < count; i++) {
        // If we found it here
        if(nodes[i].name == dir && nodes[i].is_dir) {
            vfs::currentDir = nodes[i].path;
            // Changing FS if possible
            if(curr_fs != nodes[i].fs) curr_fs = nodes[i].fs;
            
            // Adding to VFS tree
            vfs::add_node(currNode, nodes[i].name, nodes[i].inode_num, nodes[i].inode, nodes[i].fs);
            return true;
        }
    }

    vga::warning("Couldn't find directory \"%S\" in \"%S\"\n", dir, vfs::currentDir);
    return false;
}

// Inserts dir entry in block
bool insert_into_block(ext2_fs_t* fs, uint32_t block_num, const char* name, uint32_t inode_num, uint8_t* buf, bool* inserted) {
    ext2::read_block(fs, block_num, buf);

    const uint16_t needed_len = dirent_rec_len((uint8_t)strlen(name));

    uint32_t offset = 0;
    // Checking block for inodes
    while (offset < fs->block_size) {
        dir_ent_t* entry = (dir_ent_t*)(buf + offset);

        if (entry->entry_size == 0) break;

        uint16_t actual_len = dirent_rec_len(entry->name_len); // Rounding up to next multiple of 4

        // If we found an empty slot
        if (entry->inode == 0) {
            if (entry->entry_size >= needed_len) {
                uint16_t orig_len = entry->entry_size;

                entry->inode = inode_num;
                entry->name_len = (uint8_t)strlen(name);
                entry->type_indicator = EXT2_FT_DIR;

                if (orig_len > needed_len) {
                    entry->entry_size = needed_len;

                    dir_ent_t* remainder = (dir_ent_t*)((uint8_t*)entry + needed_len);
                    remainder->inode = 0;
                    remainder->name_len = 0;
                    remainder->type_indicator = 0;
                    remainder->entry_size = (uint16_t)(orig_len - needed_len);
                } else {
                    entry->entry_size = orig_len;
                }

                memcpy(entry->name, name, entry->name_len);
                // Writing back to disk
                ext2::write_block(fs, block_num, buf);
                *inserted = true;
                return true;
            }
        }

        // If the entry has extra space we'll split it
        if (entry->entry_size > actual_len) {
            uint16_t extra = (uint16_t)(entry->entry_size - actual_len);
            if (extra >= needed_len) {
                dir_ent_t* new_entry = (dir_ent_t*)((uint8_t*)entry + actual_len);

                // Setting info
                entry->entry_size = actual_len;

                new_entry->inode = inode_num;
                new_entry->name_len = (uint8_t)strlen(name);
                new_entry->type_indicator = EXT2_FT_DIR;

                uint16_t new_min = dirent_rec_len(new_entry->name_len);
                if (extra > new_min) {
                    new_entry->entry_size = new_min;

                    dir_ent_t* trailing = (dir_ent_t*)((uint8_t*)new_entry + new_min);
                    trailing->inode = 0;
                    trailing->name_len = 0;
                    trailing->type_indicator = 0;
                    trailing->entry_size = (uint16_t)(extra - new_min);
                } else {
                    new_entry->entry_size = extra;
                }

                memcpy(new_entry->name, name, new_entry->name_len);
                // Writing back to disk
                ext2::write_block(fs, block_num, buf);
                *inserted = true;
                return true;
            }
        }

        // Going to next entry
        offset += entry->entry_size;
    }

    return false;
}

// Inserts a directory entry in a directory
int insert_directory_entry(ext2_fs_t* fs, inode_t* parent_inode, const char* name, uint32_t inode_num, uint8_t* buf) {
    bool inserted = false;

    // -----------------------------------
    // Handle Direct Block
    // -----------------------------------
    for (int i = 0; i < 12; i++) {
        // Allocating block if zero
        if (parent_inode->direct_blk_ptr[i] == 0){
            parent_inode->direct_blk_ptr[i] = ext2::alloc_block(fs);
            if (parent_inode->direct_blk_ptr[i] == (uint32_t)-1) return -1;
            zero_block(fs, parent_inode->direct_blk_ptr[i], fs->block_size);
            parent_inode->size_low += fs->block_size;
        }

        // If we could insert a value into a block
        if (insert_into_block(fs, parent_inode->direct_blk_ptr[i], name, inode_num, buf, &inserted))
            return 0;
    }

    const uint32_t ptrs_per_block = fs->block_size / 4;

    // -----------------------------------
    // Handle Singly Indirect Block
    // -----------------------------------
    if (parent_inode->singly_inderect_blk_ptr == 0){
        // Allocating block if zero
        parent_inode->singly_inderect_blk_ptr = ext2::alloc_block(fs);
        if (parent_inode->singly_inderect_blk_ptr == (uint32_t)-1) return -2;
        zero_block(fs, parent_inode->singly_inderect_blk_ptr, fs->block_size);
    }

    uint32_t* singly = (uint32_t*)kcalloc(1, fs->block_size);
    ext2::read_block(fs, parent_inode->singly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(singly));

    for (uint32_t i = 0; i < ptrs_per_block; i++) {
        if (singly[i] == 0){
            // Allocating block if zero
            singly[i] = ext2::alloc_block(fs);
            if (singly[i] == (uint32_t)-1){ kfree(singly); return -2; }
            zero_block(fs, singly[i], fs->block_size);
            parent_inode->size_low += fs->block_size;
        }

        // If we could insert a value into a block
        if (insert_into_block(fs, singly[i], name, inode_num, buf, &inserted)) {
            ext2::write_block(fs, parent_inode->singly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(singly));
            kfree(singly);
            return 0;
        }
    }
    ext2::write_block(fs, parent_inode->singly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(singly)); // Persist pointer updates
    kfree(singly);

    // -----------------------------------
    // Handle Doubly Indirect Block
    // -----------------------------------
    if (parent_inode->doubly_inderect_blk_ptr == 0){
        parent_inode->doubly_inderect_blk_ptr = ext2::alloc_block(fs);
        if (parent_inode->doubly_inderect_blk_ptr == (uint32_t)-1) return -2;
        zero_block(fs, parent_inode->doubly_inderect_blk_ptr, fs->block_size);
    }

    uint32_t* doubly = (uint32_t*)kcalloc(1, fs->block_size);
    ext2::read_block(fs, parent_inode->doubly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(doubly));

    for (uint32_t i = 0; i < ptrs_per_block; i++) {
        if (doubly[i] == 0){
            // Allocating block if zero (level-1 index)
            doubly[i] = ext2::alloc_block(fs);
            if (doubly[i] == (uint32_t)-1){ kfree(doubly); return -2; }
            zero_block(fs, doubly[i], fs->block_size);
        }

        uint32_t* singly_lvl2 = (uint32_t*)kcalloc(1, fs->block_size);
        ext2::read_block(fs, doubly[i], reinterpret_cast<uint8_t*>(singly_lvl2));

        for (uint32_t j = 0; j < ptrs_per_block; j++) {
            if (singly_lvl2[j] == 0){
                // Allocating block if zero
                singly_lvl2[j] = ext2::alloc_block(fs);
                if (singly_lvl2[j] == (uint32_t)-1){ kfree(singly_lvl2); kfree(doubly); return -2; }
                zero_block(fs, singly_lvl2[j], fs->block_size);
                parent_inode->size_low += fs->block_size;
            }

            // If we could insert a value into a block
            if (insert_into_block(fs, singly_lvl2[j], name, inode_num, buf, &inserted)) {
                ext2::write_block(fs, doubly[i], reinterpret_cast<uint8_t*>(singly_lvl2));
                ext2::write_block(fs, parent_inode->doubly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(doubly));
                kfree(singly_lvl2);
                kfree(doubly);
                return 0;
            }
        }
        ext2::write_block(fs, doubly[i], reinterpret_cast<uint8_t*>(singly_lvl2));
        kfree(singly_lvl2);
    }
    ext2::write_block(fs, parent_inode->doubly_inderect_blk_ptr, reinterpret_cast<uint8_t*>(doubly));
    kfree(doubly);

    // -----------------------------------
    // Handle Triply Indirect Block
    // -----------------------------------
    if (parent_inode->triply_inderect_blk_ptr == 0){
        parent_inode->triply_inderect_blk_ptr = ext2::alloc_block(fs);
        if (parent_inode->triply_inderect_blk_ptr == (uint32_t)-1) return -2;
        zero_block(fs, parent_inode->triply_inderect_blk_ptr, fs->block_size);
    }

    uint32_t* triply = (uint32_t*)kcalloc(1, fs->block_size);
    ext2::read_block(fs, parent_inode->triply_inderect_blk_ptr, reinterpret_cast<uint8_t*>(triply));

    for (uint32_t i = 0; i < ptrs_per_block; i++) {
        if (triply[i] == 0){
            triply[i] = ext2::alloc_block(fs);
            if (triply[i] == (uint32_t)-1){ kfree(triply); return -2; }
            zero_block(fs, triply[i], fs->block_size);
        }

        uint32_t* doubly_lvl2 = (uint32_t*)kcalloc(1, fs->block_size);
        ext2::read_block(fs, triply[i], reinterpret_cast<uint8_t*>(doubly_lvl2));

        for (uint32_t j = 0; j < ptrs_per_block; j++) {
            if (doubly_lvl2[j] == 0){
                doubly_lvl2[j] = ext2::alloc_block(fs);
                if (doubly_lvl2[j] == (uint32_t)-1){ kfree(doubly_lvl2); kfree(triply); return -2; }
                zero_block(fs, doubly_lvl2[j], fs->block_size);
            }

            uint32_t* singly_lvl3 = (uint32_t*)kcalloc(1, fs->block_size);
            ext2::read_block(fs, doubly_lvl2[j], reinterpret_cast<uint8_t*>(singly_lvl3));

            for (uint32_t k = 0; k < ptrs_per_block; k++) {
                if (singly_lvl3[k] == 0){
                    // Allocating block if zero
                    singly_lvl3[k] = ext2::alloc_block(fs);
                    if (singly_lvl3[k] == (uint32_t)-1){ kfree(singly_lvl3); kfree(doubly_lvl2); kfree(triply); return -2; }
                    zero_block(fs, singly_lvl3[k], fs->block_size);
                    parent_inode->size_low += fs->block_size;
                }

                // If we could insert a value into a block
                if (insert_into_block(fs, singly_lvl3[k], name, inode_num, buf, &inserted)) {
                    ext2::write_block(fs, doubly_lvl2[j], reinterpret_cast<uint8_t*>(singly_lvl3));
                    ext2::write_block(fs, triply[i], reinterpret_cast<uint8_t*>(doubly_lvl2));
                    ext2::write_block(fs, parent_inode->triply_inderect_blk_ptr, reinterpret_cast<uint8_t*>(triply));
                    kfree(singly_lvl3);
                    kfree(doubly_lvl2);
                    kfree(triply);
                    return 0;
                }
            }
            ext2::write_block(fs, doubly_lvl2[j], reinterpret_cast<uint8_t*>(singly_lvl3));
            kfree(singly_lvl3);
        }
        ext2::write_block(fs, triply[i], reinterpret_cast<uint8_t*>(doubly_lvl2));
        kfree(doubly_lvl2);
    }
    ext2::write_block(fs, parent_inode->triply_inderect_blk_ptr, reinterpret_cast<uint8_t*>(triply));
    kfree(triply);

    return -3; // No space found
}

/// @brief Removes dir entry
/// @param parent Parent node
/// @param name Name of entry to remove
bool remove_dir_entry(treeNode* parent, data::string name) {
    if(!parent || name.empty()) return false;

    ext2_fs_t* fs = parent->data.fs;
    inode_t* parent_inode = parent->data.inode;
    if(!fs || !parent_inode) return false;

    // Allocating buffer for a single block
    uint8_t* buf = (uint8_t*)kmalloc(fs->block_size);
    if (!buf) {
        vga::warning("remove_dir_entry: out of memory\n");
        return false;
    }

    bool removed = false;
    // Itterating through direct blocks
    for(int i = 0; i < 12; i++) {

    }
}

/// @brief Removes a directory entry
/// @param parent Parent of entry to remove
/// @param name Name of entry to remove
void remove_entry(treeNode* parent, data::string name, bool is_dir) {
    // Getting parent directories entries
    int count;
    vfsNode* nodes = ext2::read_dir(parent, count);
    vfsNode node;
    bool found = false;
    // Finding entry
    for(int i = 0; i < count; i++) {
        if(nodes[i].name == name) {
            node = nodes[i]; // save matched entry
            found = true;
            break;
        }
    }
    if(!found) {
        vga::warning("rm: Couldn't find directory \"%S\" in \"%S\"\n", name, vfs::currentDir);
        return;
    }

    
}

#pragma region Terminal Functions

/// @brief Prints working directory
void ext2::pwd(void) {
    vga::printf("%S\n", vfs::currentDir);
}

/// @brief Lists directory entries
void ext2::ls(void) {
    // Getting parameter list from cmd
    int count;
    data::string* tokens = split_string_tokens(get_current_input(), count);

    bool metadata_print = false;
    if(count >= 2) {
        if(tokens[1].equals("-l")) metadata_print = true;
        else {
            vga::warning("Invalid parameter \"%S\" passed to ls\n", tokens[1]);
            return;
        }
    }

    treeNode* node = vfs::get_node(vfs::currentDir); // Retreiving node from current path
    if(!node) return;

    vfsNode* nodes = ext2::read_dir(node, count);
    if(!nodes) return;

    // Printing
    // Default listing
    if(!metadata_print)
        for(int i = 0; i < count; i++) {
            // Printing all entries instead of parent and same dir
            if(!nodes[i].name.equals(".") && !nodes[i].name.equals("..")) {
                vga::printf(nodes[i].is_dir ? PRINT_COLOR_LIGHT_BLUE | (PRINT_COLOR_BLACK << 4) : PRINT_COLOR_WHITE | (PRINT_COLOR_BLACK << 4), "%S ", nodes[i].name);
            }
        }
    // Metadata printing
    else 
        for(int i = 0; i < count; i++) {
            // Printing all entries instead of parent and same dir
            if(!nodes[i].name.equals(".") && !nodes[i].name.equals("..")) {
                if(nodes[i].inode) {
                    // Type and permissions
                    vga::printf("%S ", mode_to_string(nodes[i].inode->type_and_perm));
                    // Link counts
                    vga::printf("%u ", nodes[i].inode->hard_link_count);
                    // UID
                    vga::printf("%u ", nodes[i].inode->uid);
                    // GID
                    vga::printf("%u ", nodes[i].inode->gid);
                    // Size
                    vga::printf("%u ", nodes[i].inode->size_low);
                    // Modify time
                    vga::printf("%S ", rtc::timestamp_to_string(nodes[i].inode->last_mod_time));
                }
                // Name
                vga::printf(nodes[i].is_dir ? PRINT_COLOR_LIGHT_BLUE | (PRINT_COLOR_BLACK << 4) : PRINT_COLOR_WHITE | (PRINT_COLOR_BLACK << 4), "%S\n", nodes[i].name);
            }
        }
    vga::printf("\n");
}


/// @brief Changes directory
void ext2::cd(void) {
    // Getting directory to change to
    data::string input = get_remaining_string(get_current_input());
    if(input.empty()) {
        vga::warning("Syntax: cd <dir>\n");
        return;
    }

    int count;
    data::string* dirs = split_path_tokens(input, count);
    // Calling logic for all dirs
    for(int i = 0; i < count; i++)
        if(!change_dir(dirs[i])) return;
}


/// @brief Created a directory in a FS 
void ext2::mkdir(void) {
    // Getting directory to create
    data::string input = get_remaining_string(get_current_input());
    if(input.empty()) {
        vga::warning("Syntax: mkdir <dir>\n");
        return;
    }
    if(input[0] == '-') {
        vga::warning("Please don't start the name of the dir with '-'\n");
        return;
    }

    treeNode* node = vfs::get_node(vfs::currentDir); // Retreiving node from current path
    if(!node) return;
    vfsNode parent = node->data;
    if(!parent.fs) return;

    ext2_fs_t* fs = parent.fs;
    // Checking if we're in a Ext2 FS to create a dir
    if(!fs) {
        vga::warning("Can't create directory in \"%S\", because it's not in an Ext2 File System!\n", vfs::currentDir);
        return;
    }

    int count;
    vfsNode* nodes = ext2::read_dir(node, count);
    if(!nodes) return;
    // Checking if the dir already exists
    for(int i = 0; i < count; i++) {
        // Printing all entries instead of parent and same dir
        if(nodes[i].name == input && nodes[i].is_dir) {
            vga::warning("Directory \"%S\" already exists in \"%S\"\n", input, vfs::currentDir);
            return;
        }
    }

    // Allocating inode and block
    uint32_t inode_num = ext2::alloc_inode(fs);
    if(inode_num == (uint32_t)-1) {
        vga::error("Couldn't create directory do to inode allocation fail!\n");
        return;
    }
    uint32_t block_num = ext2::alloc_block(fs);
    if(block_num == (uint32_t)-1) {
        vga::error("Couldn't create directory do to block allocation fail!\n");
        return;
    }
    
    // Getting and initializing newly allocated inode
    inode_t* inode = load_inode(fs, inode_num);
    memset(inode, 0, sizeof(inode_t));
    
    inode->type_and_perm = EXT2_S_IFDIR | DEFAULT_DIR_PERMS;
    inode->uid = 0;
    inode->gid = 0;
    inode->size_low = fs->block_size;
    inode->create_time = inode->last_access_time = inode->last_mod_time = rtc::get_unix_timestamp();
    inode->hard_link_count = 2;  // '.' and '..'
    inode->disk_sect_count = (fs->block_size / 512);
    inode->direct_blk_ptr[0] = block_num;

    uint8_t* buf = (uint8_t*)kcalloc(1, fs->block_size);

    // Create directory entries: '.' and '..'
    dir_ent_t* dot = (dir_ent_t*)buf;
    dot->inode = inode_num;
    dot->name_len = 1;
    dot->type_indicator = EXT2_FT_DIR;
    dot->name[0] = '.';
    dot->entry_size = dirent_rec_len(dot->name_len);

    dir_ent_t* dotdot = (dir_ent_t*)((uint8_t*)dot + dot->entry_size);
    dotdot->inode = parent.inode_num;
    dotdot->name_len = 2;
    dotdot->type_indicator = EXT2_FT_DIR;
    dotdot->name[0] = '.';
    dotdot->name[1] = '.';
    dotdot->entry_size = (uint16_t)(fs->block_size - dot->entry_size); // Rest of block

    // Writing to disk
    write_block(fs, block_num, buf);

    // Insert into parent dir
    int res = insert_directory_entry(fs, parent.inode, input, inode_num, buf);
    if (res != 0) {
        kfree(buf);
        return;
    }

    parent.inode->hard_link_count++;
    parent.inode->last_mod_time = rtc::get_unix_timestamp();

    // Write back inodes
    write_inode(fs, inode_num, inode);
    write_inode(fs, parent.inode_num, parent.inode);
    inode = load_inode(fs, inode_num);

    // Adding to VFS
    vfs::add_node(vfs::get_node(vfs::currentDir), input, inode_num, inode, fs);

    kfree(buf);
    return;
}

/// @brief Removes dir entry
void ext2::rm(void) {
    // Getting params
    int count; data::string* tokens = split_string_tokens(get_current_input(), count);

    if(count == 3) {
        if(tokens[1] != "-rf" && tokens[1] != "-r") goto invalid_params;
        bool forced_remove = false;
        // Checking if we'll remove it forcefully
        if(tokens[1] == "-rf") forced_remove = true;


        return;
    }
    else if(count == 2) {
        if(tokens[1][0] == '-' && tokens[1] == "-h") {
            vga::printf("rm <file> - Deletes file (doesn't work with directories)\n");
            vga::printf("rm -r <dir> - Deletes directory (recursively deletes contents)\n");
            vga::printf("rm -rf <dir> - Force deletes directory (Deletes contents without any warnings)\n");
            return;
        }
        
        // rm <file>
        remove_entry(vfs::get_node(vfs::currentDir), tokens[1], false);
        return;
    }

    invalid_params:
    vga::warning("Invalid parameters passed to rm! Try rm -h\n");
    return;
}

#pragma endregion
