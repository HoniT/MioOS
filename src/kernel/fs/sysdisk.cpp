// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// sysdisk.cpp
// Finds and sets up system disk.
// ========================================

#include <fs/sysdisk.hpp>
#include <multiboot.hpp>
#include <x86/interrupts/kernel_panic.hpp>
#include <graphics/vga_print.hpp>
#include <drivers/vga.hpp>
#include <device.hpp>
#include <fs/ext/ext2.hpp>
#include <fs/ext/vfs.hpp>
#include <x86/mbr.hpp>
#include <lib/data/list.hpp>
#include <lib/data/string.hpp>

data::list<data::string> sys_dirs;

bool initialized = false;
void init_sys_files(void) {
    // List of system directories that are needed
    sys_dirs = data::list<data::string>::of("mnt", "home");
    initialized = true;
}

void init_sysdisk(ext2_fs_t* fs, ata::device_t* dev) {
    // Mounting system disk as VFS root
    treeNode* node = vfs::init(fs);

    // Creating any needed directories if needed
    data::list<vfsNode> nodes = ext2::read_dir(node);
    for(data::string dir : sys_dirs) {
        bool found = false;
        for(vfsNode node : nodes) if(node.name == dir) found = true;
        if(!found) ext2::make_dir(dir, node->data, node, RESTRICTED_PERMS);
    }
    ext2::find_other_ext2_fs(dev);

    kprintf(LOG_INFO, "Implemented VFS to system disk\n");
}

void init_sysdisk(ext2_fs_t* fs, ahci::device_t* dev) {
    // Mounting system disk as VFS root
    treeNode* node = vfs::init(fs);

    // Creating any needed directories if needed
    data::list<vfsNode> nodes = ext2::read_dir(node);
    for(data::string dir : sys_dirs) {
        bool found = false;
        for(vfsNode node : nodes) if(node.name == dir) found = true;
        if(!found) ext2::make_dir(dir, node->data, node, RESTRICTED_PERMS);
    }
    ext2::find_other_ext2_fs(dev);

    kprintf(LOG_INFO, "Implemented VFS to system disk\n");
}

/// @brief Finds disk that contains the system
void sysdisk::find_sysdisk() {
    /* This is a primitive version (can't differentiate drives if multiple of them has MioOS).
     * In later stages of the OS (when we'll add instalation) we will use UUIDs */
    // Initializing sys file names if not already
    if(!initialized) init_sys_files();

    // Checking ATA
    for(int i = 0; i < 4; i++) {
        if(ata_devices[i]) {
            mbr_t* mbr = (mbr_t*)kmalloc(sizeof(mbr_t));
            if(!mbr::read_mbr(ata_devices[i], mbr)) {
                kfree(mbr);
                continue;
            }
            kfree(mbr);
            ext2_fs_t* fs;
            if(!(fs = ext2::init_ext2_device(ata_devices[i], true))) continue;

            init_sysdisk(fs, ata_devices[i]);
            return;
        }
    }

    for(ahci::device_t* dev : ahci_devices) {
        mbr_t* mbr = (mbr_t*)kmalloc(sizeof(mbr_t));
        if(!mbr::read_mbr(dev, mbr)) {
            kfree(mbr);
            continue;
        }
        kfree(mbr);
        ext2_fs_t* fs;
        if(!(fs = ext2::init_ext2_device(dev, true))) continue;

        init_sysdisk(fs, dev);
        return;
    }

    kprintf(LOG_WARNING, "System disk not found!\n");
    kprintf(LOG_INFO, RGB_COLOR_LIGHT_BLUE, "Entering mobile mode\n");
    vfs::init();
    ext2::find_ext2_fs();
}
