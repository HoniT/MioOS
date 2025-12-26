# Ext2 Filesystem & Virtual File System (VFS)

This document details the implementation of the **MioOS** storage subsystem. It consists of a high-level **Virtual File System (VFS)** that provides a unified tree structure, and a low-level **Ext2 Driver** that manages physical data storage, permissions, and block allocation.

## Architecture Overview

The storage system operates in two layers:
1.  **VFS Layer (`vfs.cpp`):** Abstracts physical devices into a single directory tree rooted at `/`. It caches directory nodes to reduce disk I/O.
2.  **Ext2 Driver (`ext2.cpp`, `block.cpp`, `inode.cpp`):** Handles the physical layout of the Ext2 filesystem, including inodes, bitmaps, and complex block addressing.

---

## 1. Virtual File System (VFS)

The VFS maintains a cached tree of `vfsNode` structures. It allows the kernel to navigate paths like `/mnt/hda/home/user` without constantly parsing raw directory blocks on every access.

### Core Components
* **Root Node:** The filesystem root `/`.
* **Mount Points:** Physical Ext2 partitions are mounted to virtual nodes (e.g., devices detected by ATA are mounted under `/mnt/`).
* **Context:** The VFS tracks the current working directory (`currentDir`), User ID (`currUid`), and Group ID (`currGid`).

### VFS API Reference

| Function | Description |
| :--- | :--- |
| `vfs::init(fs)` | Initializes the VFS tree and mounts the root filesystem. |
| `vfs::mount_dev(name, inode, fs)` | Mounts a physical device's root inode to a virtual path. |
| `vfs::get_node(path)` | Traverses the virtual tree to return the `treeNode*` for a specific path. |
| `vfs::find_inode(fs, path)` | Helper to locate the physical `inode_t` for a path by traversing directory tokens. |
| `vfs::add_node(...)` | Adds a discovered file/directory to the VFS cache. |

---

## 2. Ext2 File Operations

The Ext2 driver manages reading and writing file data. It supports complex addressing to handle files larger than a single block.



### Block Addressing Support
The driver implements the full Ext2 block pointer specification:
* **Direct Blocks (0-11):** Points directly to data.
* **Singly Indirect:** Points to a block of pointers.
* **Doubly Indirect:** Points to a block of singly indirect pointers.
* **Triply Indirect:** Points to a block of doubly indirect pointers.

### File API Reference

| Function | Description |
| :--- | :--- |
| `get_file_contents(path)` | Reads the entire file into a `large_string`. Automatically handles traversing the indirect block tree to assemble fragmented data. Updates `last_access_time`. |
| `write_file_content(path, data, overwrite)` | Writes data to a file. <br>• **Overwrite:** Replaces content from offset 0.<br>• **Append:** Adds to the end.<br>Automatically allocates new blocks (direct through triple indirect) as needed and updates file size/modification time. |
| `make_file(name, parent, node, perms)` | Creates a new file inode, links it to the parent directory, and adds it to the VFS. |

---

## 3. Directory Management

Directories in Ext2 are special files containing a list of variable-length entries (`dir_ent_t`).

### Directory API Reference

| Function | Description |
| :--- | :--- |
| `read_dir(node)` | Parses the binary directory block and returns a list of files/folders. Used by `ls` or to populate the VFS cache. |
| `make_dir(name, parent, node, perms)` | **`mkdir` logic.**<br>1. Allocates a new inode (Type `DIR`).<br>2. Creates `.` and `..` entries.<br>3. Inserts the new entry into the parent directory.<br>4. Updates link counts. |
| `change_dir(path)` | **`cd` logic.** Verifies the target exists and the user has **Execute** (`x`) permissions before updating the `currentDir`. |
| `remove_entry(node)` | **`rm` logic.**<br>1. Checks **Write** permissions.<br>2. Unlinks the entry from the directory.<br>3. Decrements link count. If 0, calls `free_inode` and `free_blocks`. |

---

## 4. Physical Memory Management

This layer handles the allocation and deallocation of physical resources on the disk using Bitmaps.

### Block Manager (`block.cpp`)
Manages the data blocks. It interacts with the **Block Group Descriptor (BGD)** table to find free space.

* **Allocation:** `alloc_block` scans the block bitmap of a block group to find a free bit. It uses a "First Fit" strategy.
* **Deallocation:** `free_blocks` recursively walks an inode's structure (including deep indirect trees) to free every sector used by a file, updating the bitmaps and superblock counts.

### Inode Manager (`inode.cpp`)
Manages the file metadata structures.

* **Allocation:** `alloc_inode` scans the inode bitmap to find a free ID (skipping reserved inodes).
* **Persistence:** `write_inode` calculates the exact sector offset of an inode in the Inode Table and flushes the struct to disk.

---

## 5. Permissions & Security

The system enforces standard Unix-style permissions (Owner/Group/Other).

| Function | Description |
| :--- | :--- |
| `get_perms(inode, uid, gid)` | Returns a struct containing `read`, `write`, and `execute` bools based on the current user's relation to the file owner. |

**Enforcement Rules:**
* **`cd`:** Requires `Execute` permission on the target directory.
* **`rm` / `mkdir`:** Requires `Write` and `Execute` permissions on the **parent** directory.
* **`write`:** Requires `Write` permission on the file.
* **`ls`, `cat`:** Requires `Read` permission on the file/directory.