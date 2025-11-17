// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// sysdisk.cpp
// Finds and sets up system disk.
// ========================================

#include <fs/sysdisk.hpp>
#include <kernel_main.hpp>
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
    sys_dirs = data::list<data::string>();
    sys_dirs.push_back("mnt");

    initialized = true;
}

/// @brief Finds and sets up system disk
/// @param mbi GRUB multiboot info
void sysdisk::get_sysdisk(void* mbi) {
    // Initializing sys file names if not already
    if(!initialized) init_sys_files();

    // Get boot device information
    multiboot_tag_bootdev* bootdev = Multiboot2::get_bootdev(mbi);
    // Searching for system disk to mount as FS root
    int ata_index = bootdev->biosdev - 0x80;
    if (ata_index < 0 || ata_index >= 4) {
        kprintf(LOG_WARNING, "Boot device BIOS number (%x) out of ATA range! Device could be AHCI\n", bootdev->biosdev);
        kprintf(LOG_INFO, RGB_COLOR_LIGHT_RED, "Entering mobile mode");
        vfs::init();
        ext2::find_ext2_fs();
        return;
    }
    ata::device_t* dev = ata_devices[ata_index];
    if(!dev) {
        // TODO: Check AHCI devices if ATA doesnt exist
        kprintf(LOG_WARNING, "Couldn't find system disk for BIOS boot device number %x!\n", bootdev->biosdev);
        kprintf(LOG_INFO, RGB_COLOR_LIGHT_RED, "Entering mobile mode");
        vfs::init();
        ext2::find_ext2_fs();
        return;
    }
    // Checking MBR for 0xAA55
    mbr_t* mbr = (mbr_t*)kmalloc(sizeof(mbr_t));
    if(!mbr::read_mbr(dev, mbr)) {
        // TODO: Check AHCI devices if ATA isn't bootable
        kprintf(LOG_WARNING, "System disk isn't bootable! (LGA 0 ends with 0x%h)\n", mbr->signature);
        kprintf(LOG_INFO, RGB_COLOR_LIGHT_RED, "Entering mobile mode");
        vfs::init();
        ext2::find_ext2_fs();
        return;
    }
    kfree(mbr);
    // Initializing Ext2
    ext2_fs_t* fs = ext2::init_ext2_device(dev, true);
    if(!fs) {
        // TODO: Check AHCI devices if ATA isn't bootable
        kprintf(LOG_WARNING, "System disk doesn't have a valid Ext file system!\n");
        kprintf(LOG_INFO, RGB_COLOR_LIGHT_RED, "Entering mobile mode");
        vfs::init();
        ext2::find_ext2_fs();
        return;
    }

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
