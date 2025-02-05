# Reinitializing a USB Drive After Writing a Bootable Image

When you create a bootable USB with a raw `.iso` file, your system may no longer recognize the USB drive in File Explorer (Windows) or File Manager (Linux). This guide explains how to reinitialize the USB to restore it to its original state.

---

## Warnings
- **Back up important data**: The process will erase all data on the USB drive.
- **Correct USB identification**: Be careful when selecting the USB drive to avoid accidentally wiping the wrong disk.

---

## Reinitialize USB Drive on Windows

1. **Open Disk Management**
   - Press `Win + R`, type `diskmgmt.msc`, and hit Enter.

2. **Identify the USB Drive**
   - Look for your USB drive in the list of disks. It will usually have "Unallocated" or "RAW" as its file system.

3. **Reinitialize the Drive Using `diskpart`**
   - Open Command Prompt as Administrator:
     ```cmd
     diskpart
     ```
   - List all disks:
     ```cmd
     list disk
     ```
   - Identify the USB drive (e.g., `Disk 1`) and select it:
     ```cmd
     select disk X
     ```
     Replace `X` with the number of your USB drive.

   - Clean the drive:
     ```cmd
     clean
     ```

   - Create a new partition:
     ```cmd
     create partition primary
     ```

   - Format the drive as FAT32:
     ```cmd
     format fs=fat32 quick
     ```

   - Assign a drive letter:
     ```cmd
     assign
     ```

   - Exit diskpart:
     ``` cmd
     exit
     ```

4. **Verify the USB Drive**
   - Open File Explorer, and the USB drive should now appear and be usable.

---

## Reinitialize USB Drive on Linux

1. **Insert the USB Drive**
   - Plug your USB drive into the system.

2. **Identify the USB Drive**
   - Open a terminal and run:
     ```bash
     lsblk
     ```
   - Identify your USB drive (e.g., `/dev/sdX`, where `X` is the device letter).

3. **Uninstall the Drive**
   - Unmount any mounted partitions:
     ```bash
     sudo umount /dev/sdX*
     ```

4. **Use `fdisk` to Reinitialize**
   - Open `fdisk`:
     ```bash
     sudo fdisk /dev/sdX
     ```
   - Delete all existing partitions:
     - Type `d` to delete a partition. Repeat until all partitions are deleted.
   - Create a new partition:
     - Type `n` to create a new partition.
     - Follow the prompts to accept the defaults.
   - Write the changes and exit:
     - Type `w` and hit Enter.

5. **Format the USB Drive**
   - Format the new partition as FAT32:
     ```bash
     sudo mkfs.vfat /dev/sdX1
     ```

6. **Verify the USB Drive**
   - Reinsert the USB, and it should now be usable.

---

## Additional Notes

- If your USB drive is still not recognized, double-check the device identifier (e.g., `/dev/sdX` or `Disk X`) to ensure you worked on the correct drive.
- You can also use GUI tools like GParted on Linux or Rufus on Windows for reinitializing the USB drive.
