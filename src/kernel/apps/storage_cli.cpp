// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// storage_cli.cpp
// Storage/File system CLI commands
// ========================================

#include <apps/storage_cli.hpp>
#include <apps/kterminal.hpp>
#include <drivers/ata.hpp>
#include <device.hpp>
#include <fs/ext/vfs.hpp>
#include <drivers/vga.hpp>
#include <drivers/rtc.hpp>
#include <lib/path_util.hpp>
#include <lib/data/list.hpp>
#include <lib/data/string.hpp>

void cmd::storage_cli::register_app() {
    cmd::register_command("read_ata", read_ata, " -dev <device_index> -sect <sector_index>", " - Prints a given sector of a given ATA device");
    cmd::register_command("lsata", list_ata, "", " - Lists available ATA devices");
    cmd::register_command("read_ahci", read_ahci, " -dev <device_index> -sect <sector_index>", " - Prints a given sector of a given AHCI device");
    cmd::register_command("lsahci", list_ahci, "", " - Lists available AHCI devices");
    cmd::register_command("pwd", pwd, "", " - Prints working directory");
    cmd::register_command("ls", ls, "", " - Lists entries of the current directory");
    cmd::register_command("cd", cd, " <dir>", " - Changes directory to given dir");
    cmd::register_command("mkdir", mkdir, " <dir>", " - Creates a directory in the current dir");
    cmd::register_command("mkfile", mkfile, " <file>", " - Creates a file in the current dir");
    cmd::register_command("rm", rm, " <file>", " - Removes (deletes) a directory/directory entry");
    cmd::register_command("cat", cat, " <file>", " - Prints file contents");
    cmd::register_command("write", write_to_file, " <file> <content>", " - Writes something to a file");
    cmd::register_command("append", append_to_file, " <file> <content>", " - Appends something to a file");
}

void cmd::storage_cli::read_ata() {
    data::list<data::string> params = cmd::storage_cli::get_params();

    if(params.count() != 4 || !params.at(0).equals("-dev") || !params.at(2).equals("-sect")) {
        kprintf(LOG_INFO, "Syntax: read_ata -dev <device_index> -sect <sector_index>\n");
        return;
    }

    int device_index = str_to_int(params.at(1));
    if(device_index < 0 || device_index >= 4) {
        kprintf(LOG_INFO, "Please use an integer (0-3) as the device index in decimal format.\n");
        return;
    }
    ata::device_t* device = ata_devices[device_index];
    if(!device) {
        kprintf(LOG_INFO, "read_ata: Invalid device\n");
        return;
    }

    int sector_index = str_to_int(params.at(3));
    if(sector_index < 0 || sector_index >= device->total_sectors) {
        kprintf(LOG_INFO, "Please use a decimal integer as the sector index. Make sure it's in the given devices maximum sector count: %u\n", device->total_sectors);
        return;
    }

    uint16_t buffer[256];
    if(pio_28::read_sector(device, sector_index, buffer))
        for(uint16_t i = 0; i < 256; i++) 
            kprintf("%hx ", buffer[i]);
}

void cmd::storage_cli::read_ahci() {
    data::list<data::string> params = cmd::storage_cli::get_params();

    if(params.count() != 4 || !params.at(0).equals("-dev") || !params.at(2).equals("-sect")) {
        kprintf(LOG_INFO, "Syntax: read_ahci -dev <device_index> -sect <sector_index>\n");
        return;
    }

    ahci::device_t* device = ahci_devices[str_to_int(params.at(1))];
    if(!device) {
        kprintf(LOG_INFO, "read_ahci: Invalid device\n");
        return;
    }
    int sector_index = str_to_int(params.at(3));
    if(sector_index < 0 || sector_index >= device->total_sectors) {
        kprintf(LOG_INFO, "Please use a decimal integer as the sector index. Make sure it's in the given devices maximum sector count: %u\n", device->total_sectors);
        return;
    }

    uint16_t buffer[256];
    if(device->ahci->read(device->port, sector_index, 1, buffer))
        for(uint16_t i = 0; i < 256; i++) 
            kprintf("%hx ", buffer[i]);
}

void cmd::storage_cli::list_ata() {
    for(int i = 0; i < 4; i++) {
        if(!ata_devices[i]) continue;
        ata::device_t* device = ata_devices[i]; 
        kprintf("\nModel: %s, serial: %s, firmware: %s, total sectors: %u, lba_support: %u, dma_support: %u ", 
            device->model, device->serial, device->firmware, device->total_sectors, (uint32_t)device->lba_support, (uint32_t)device->dma_support);
        kprintf("IO information: bus: %s, drive: %s\n", device->bus == ata::Bus::Primary ? "Primary" : "Secondary", device->drive == ata::Drive::Master ? "Master" : "Slave");
    }
}

void cmd::storage_cli::list_ahci() {
    for(ahci::device_t* device : ahci_devices) {
        kprintf("\nModel: %s, serial: %s, firmware: %s, total sectors: %u ", 
            device->model, device->serial, device->firmware, device->total_sectors);
        device->ahci->get_pci_dev()->log_pci_info();
    }
}

void cmd::storage_cli::pwd() {
    kprintf("%S\n", vfs::currentDir);
}

void cmd::storage_cli::ls() {
    data::list<data::string> params = cmd::storage_cli::get_params();

    bool metadata_print = false, all_print = false;
    if(params.count() >= 1) {
        if(params.at(0).at(0) == '-') {
            if(params.at(0).includes("a")) all_print = true;
            if(params.at(0).includes("l")) metadata_print = true;
        }
        else {
            kprintf(LOG_INFO, "ls: Invalid parameter \"%S\" passed to ls\n", params.at(0));
            return;
        }
    }

    treeNode* node = vfs::get_node(vfs::currentDir); // Retreiving node from current path
    if(!node) return;
    ext2::read_dir(node).~list();

    data::list<vfsNode> nodes = vfs_tree.get_children(node);
    // Printing
    // Default listing
    if(!metadata_print) {
        for(vfsNode node : nodes) {
            // Permission denied
            if(node.inode && !ext2::get_perms(node.inode, vfs::currUid, vfs::currGid).read) continue;
            // Printing all entries instead of parent and same dir
            if((all_print && (node.name.equals(".") || node.name.equals(".."))) || !(node.name.equals(".") || node.name.equals(".."))) 
                kprintf(node.is_dir ? RGB_COLOR_LIGHT_BLUE : RGB_COLOR_WHITE, "%S ", node.name);
        }
        kprintf("\n");
    }
    // Metadata printing
    else 
        for(vfsNode node : nodes) {
            // Permission denied
            if(node.inode && !ext2::get_perms(node.inode, vfs::currUid, vfs::currGid).read) continue;
            if((all_print && (node.name.equals(".") || node.name.equals(".."))) || !(node.name.equals(".") || node.name.equals(".."))) {
                // Printing all entries instead of parent and same dir
                if(node.inode) {
                    // Type and permissions
                    kprintf("%S ", ext2::mode_to_string(node.inode->type_and_perm));
                    // Link counts
                    kprintf("%u ", node.inode->hard_link_count);
                    // UID
                    kprintf("%u ", node.inode->uid);
                    // GID
                    kprintf("%u ", node.inode->gid);
                    // Size
                    kprintf("%u ", node.inode->size_low);
                    // Inode num
                    kprintf("%u ", node.inode_num);
                    // Modify time
                    kprintf("%S ", rtc::timestamp_to_string(node.inode->last_mod_time));
                }
                else kprintf("(Couldn't recognize File System / Inode) ");
                // Name
                kprintf(node.is_dir ? RGB_COLOR_LIGHT_BLUE : RGB_COLOR_WHITE, "%S\n", node.name);
            }
        }
}

void cmd::storage_cli::cd() {
    data::list<data::string> params = cmd::storage_cli::get_params();

    // Getting directory to change to
    data::string input = params.at(0);
    if(input.empty()) {
        kprintf(LOG_INFO, "cd: Syntax: cd <dir>\n");
        return;
    }

    int count;
    data::string* dirs = split_path_tokens(input, count);
    // Calling logic for all dirs
    for(int i = 0; i < count; i++)
        if(!ext2::change_dir(dirs[i])) return;
    for(int i = 0; i < count; i++) dirs[i].~string();
    kfree(dirs);
}

void cmd::storage_cli::mkdir() {
    data::string dir = cmd::storage_cli::get_params().at(0);
    if(dir.empty()) {
        kprintf(LOG_INFO, "mkdir: Syntax: mkdir <dir>\n");
        return;
    }
    if(!ext2::curr_fs) {
        kprintf(LOG_WARNING, "mkdir: You are not in a valid Ext2 file system\n");
        return;
    }
    if(dir.includes("/")) {
        kprintf(LOG_WARNING, "mkdir: Please don't use '/' in a directory name\n");
        return;
    }

    treeNode* node = vfs::get_node(vfs::currentDir); // Retreiving node from current path
    if(!node) return;
    vfsNode parent = node->data;
    if(!parent.fs) return;

    ext2::make_dir(dir, parent, node, DEFAULT_PERMS);
}

void cmd::storage_cli::mkfile() {
    data::string file = cmd::storage_cli::get_params().at(0);
    if(file.empty()) {
        kprintf(LOG_INFO, "mkfile: Syntax: mkfile <file>\n");
        return;
    }
    if(!ext2::curr_fs) {
        kprintf(LOG_WARNING, "mkfile: You are not in a valid Ext2 file system\n");
        return;
    }
    if(file.includes("/")) {
        kprintf(LOG_WARNING, "mkfile: Please don't use '/' in a file name\n");
        return;
    }

    treeNode* node = vfs::get_node(vfs::currentDir); // Retreiving node from current path
    if(!node) return;
    vfsNode parent = node->data;
    if(!parent.fs) return;

    ext2::make_file(file, parent, node, DEFAULT_PERMS);
}

void cmd::storage_cli::rm() {
    if(!ext2::curr_fs) {
        kprintf(LOG_WARNING, "rm: You are not in a valid Ext2 file system\n");
        return;
    }

    // Getting params
    data::list<data::string> params = cmd::storage_cli::get_params();
    treeNode* parent = vfs::get_node(vfs::currentDir);

    if(params.count() == 2) {
        if(params.at(0) != "-r") goto invalid_params;

        // Reading dir to find the object we want to remove
        data::list<vfsNode> list = ext2::read_dir(parent);
        list.~list();
        data::string name = params.at(1);
        int count; treeNode** nodes = vfs_tree.find_children_by_predicate(parent, [name](vfsNode node){ return node.name == name;}, count);
        if(!nodes || count == 0) {
            kprintf(LOG_INFO, "rm: Couldn't find dir \"%S\" in \"%S\"\n", name, vfs::currentDir);
            return;
        }
        for(int i = 0; i < count; i++) {
            // Found a valid node
            if(nodes[i]->data.is_dir) {
                // rm -r <dir>
                // Traversing directory recursively and deleting all contents
                vfs_tree.traverse(nodes[i], ext2::remove_entry);
                // Removing from VFS tree
                vfs_tree.delete_subtree(nodes[i]);
                return;
            }
        }
        kprintf(LOG_INFO, "rm: The object (\"%S\") to delete is a file! Please use rm <file>\n", name);
        return;
    }
    else if(params.count() == 1) {
        // Reading dir to find the object we want to remove
        data::list<vfsNode> list = ext2::read_dir(parent);
        list.~list();
        data::string name = params.at(0);
        int count; treeNode** nodes = vfs_tree.find_children_by_predicate(parent, [name](vfsNode node){ return node.name == name; }, count);
        params.clear();
        if(!nodes || count == 0) {
            kprintf(LOG_INFO, "rm: Couldn't find file \"%S\" in \"%S\"\n", name, vfs::currentDir);
            return;
        }
        for(int i = 0; i < count; i++) {
            // Found a valid node
            if(!nodes[i]->data.is_dir) {
                // rm <file>
                ext2::remove_entry(nodes[i]);
                // Removing from VFS tree
                vfs_tree.delete_subtree(nodes[i]);
                return;
            }
        }
        
        // We need to recursively delete a directory
        kprintf(LOG_INFO, "rm: The object (\"%S\") to delete is a directory! Please use rm -r <dir>\n", name);
        return;
    }

    invalid_params:
    kprintf(LOG_INFO, "rm: Invalid parameters passed to rm!\n");
    kprintf("rm <file> - Deletes file (doesn't work with directories)\n");
    kprintf("rm -r <dir> - Deletes directory (recursively deletes contents)\n");
    params.clear();
    return;
}

void cmd::storage_cli::cat() {
    data::list<data::string> params = cmd::storage_cli::get_params();

    // Checking params
    if(params.count() != 1) {
        kprintf(LOG_INFO, "cat: Syntax: cat <file>\n");
        return;
    }
    if(!ext2::curr_fs) {
        kprintf(LOG_WARNING, "cat: You are not in a valid Ext2 file system\n");
        return;
    }

    // Resolve the path
    data::string path(vfs::currentDir);
    path.append(params.at(0));
    data::large_string file = ext2::get_file_contents(path);
    kprintf("%S\n", file);
}

void cmd::storage_cli::write_to_file() {
    data::list<data::string> params = cmd::storage_cli::get_params(ArgStrategy::SplitHead);

    // Checking params
    if(params.count() < 1) {
        kprintf(LOG_INFO, "write: Syntax: write_file <file> <content>\n");
        return;
    }

    // Resolve the path
    data::string path(vfs::currentDir);
    path.append(params.at(0));
    ext2::write_file_content(path, params.at(1));
}

void cmd::storage_cli::append_to_file() {
    data::list<data::string> params = cmd::storage_cli::get_params(ArgStrategy::SplitHead);
    
    // Checking params
    if(params.count() < 1) {
        kprintf(LOG_INFO, "append: Syntax: write_file <file> <content>\n");
        return;
    }

    // Resolve the path
    data::string path(vfs::currentDir);
    path.append(params.at(0));
    ext2::write_file_content(path, params.at(1), false);
}
