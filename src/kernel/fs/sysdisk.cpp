// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// sysdisk.cpp
// Finds and sets up system disk.
// ========================================

#include <fs/sysdisk.hpp>
#include <interrupts/kernel_panic.hpp>
#include <device.hpp>
#include <fs/ext/ext2.hpp>
#include <fs/ext/vfs.hpp>
#include <mbr.hpp>
#include <lib/data/list.hpp>
#include <lib/data/string.hpp>

data::list<data::string> sys_dirs;

bool initialized = false;
void init_sys_files(void) {
    // List of system directories that are needed
    sys_dirs = data::list<data::string>();
    sys_dirs.push_back("dev");
    sys_dirs.push_back("mnt");

    initialized = true;
}

/// @brief Finds and sets up system disk
/// @param mbi GRUB multiboot info
void sysdisk::get_sysdisk(const multiboot_info* mbi) {
    vga_coords coords = vga::set_init_text("Getting system disk ready");

    // Initializing sys file names if not already
    if(!initialized) init_sys_files();

    // Searching for system disk to mount as FS root
    uint8_t bios_drive = (mbi->boot_device >> 24) & 0xFF;
    int ata_index = bios_drive - 0x80;
    if (ata_index < 0 || ata_index >= 4) {
        vga::set_init_text_answer(coords, false);
        vga::warning("Boot device BIOS number (%x) out of ATA range! Device could be AHCI\n", bios_drive);
        vga::printf(PRINT_COLOR_LIGHT_GREEN | (PRINT_COLOR_BLACK << 4), "GOING INTO MOBILE MODE");
        vfs::init();
        ext2::find_ext2_fs();
        return;
    }
    ata::device_t* dev = ata_devices[ata_index];
    if(!dev) {
        // TODO: Check AHCI devices if ATA doesnt exist
        vga::set_init_text_answer(coords, false);
        vga::warning("Couldn't find system disk for BIOS boot device number %x!\n", bios_drive);
        vga::printf(PRINT_COLOR_LIGHT_GREEN | (PRINT_COLOR_BLACK << 4), "GOING INTO MOBILE MODE");
        vfs::init();
        ext2::find_ext2_fs();
        return;
    }
    // Checking MBR for 0xAA55
    mbr_t* mbr = (mbr_t*)kmalloc(sizeof(mbr_t));
    if(!mbr::read_mbr(dev, mbr)) {
        // TODO: Check AHCI devices if ATA isn't bootable
        vga::set_init_text_answer(coords, false);
        vga::warning("System disk isn't bootable! (LGA 0 ends with 0x%h)\n", mbr->signature);
        vga::printf(PRINT_COLOR_LIGHT_GREEN | (PRINT_COLOR_BLACK << 4), "GOING INTO MOBILE MODE");
        vfs::init();
        ext2::find_ext2_fs();
        return;
    }
    // Initializing Ext2
    ext2_fs_t* fs = ext2::init_ext2_device(dev, true);
    if(!fs) {
        // TODO: Check AHCI devices if ATA isn't bootable
        vga::set_init_text_answer(coords, false);
        vga::warning("System disk doesn't have a valid Ext file system!\n");
        vga::printf(PRINT_COLOR_LIGHT_GREEN | (PRINT_COLOR_BLACK << 4), "GOING INTO MOBILE MODE");
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
    vga::set_init_text_answer(coords, true);
}
