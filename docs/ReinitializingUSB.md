# Reinitializing a USB Drive After Writing a Bootable Image

When you create a bootable USB with a raw `.iso` or binary file, the drive's partition table is often overwritten with a specialized format. This can cause the system to "lose" the drive—it may not appear in File Explorer (Windows) or may show up as having the wrong storage capacity.

This guide explains how to completely wipe and restore the USB drive to a standard, usable state.

---

## Warnings

* **DATA LOSS:** This process will **permanently erase** everything on the USB drive.
* **IDENTIFICATION:** You must be absolutely certain which disk number/identifier corresponds to your USB. **Selecting the wrong disk will wipe your computer's hard drive.**

---

## Reinitialize USB Drive on Windows

The standard Windows "Format" option often fails on bootable USBs. We will use the command-line tool `diskpart` to reset the partition structure.

1.  **Open Command Prompt as Administrator**
    * Press `Win + S`, type **cmd**, right-click "Command Prompt", and select **Run as Administrator**.

2.  **Start Diskpart**
    Enter the disk partition utility:
    ```cmd
    diskpart
    ```

3.  **List and Select the Drive**
    * View all connected disks:
        ```cmd
        list disk
        ```
    * **Crucial Step:** Identify your USB drive based on the **Size** column (e.g., 14 GB for a 16GB drive).
    * Select the drive (replace `X` with your USB's number, e.g., `Disk 2`):
        ```cmd
        select disk X
        ```

4.  **Wipe and Reformat**
    Run the following commands one by one:

    * **Wipe the partition table** (This deletes everything):
        ```cmd
        clean
        ```
    * **Create a new primary partition**:
        ```cmd
        create partition primary
        ```
    * **Format as FAT32** (Compatible with everything):
        ```cmd
        format fs=fat32 quick
        ```
    * **Assign a drive letter** (Makes it visible in Explorer):
        ```cmd
        assign
        ```

5.  **Exit**
    ```cmd
    exit
    ```
    Your USB drive should now appear in File Explorer as a normal, empty drive.

---

## Reinitialize USB Drive on Linux

We will use `fdisk` to create a fresh partition table and `mkfs` to format it.

1.  **Identify the USB Drive**
    Plug in the USB and check the device list:
    ```bash
    lsblk
    ```
    Identify your drive (e.g., `/dev/sdb`). **Do not use `/dev/sda`** (usually your OS).

2.  **Unmount the Drive**
    If the OS auto-mounted any partitions, unmount them (replace `/dev/sdX` with your drive identifier):
    ```bash
    sudo umount /dev/sdX*
    ```

3.  **Reset Partition Table with `fdisk`**
    Open the drive in fdisk:
    ```bash
    sudo fdisk /dev/sdX
    ```

    Inside the `fdisk` prompt, type the following letters in order:
    1.  **`o`** : Creates a new, empty DOS partition table (this instantly wipes the old hybrid ISO structure).
    2.  **`n`** : Creates a new partition.
        * Press **Enter** (Select 'primary').
        * Press **Enter** (Default partition number).
        * Press **Enter** (First sector).
        * Press **Enter** (Last sector / use full disk).
    3.  **`w`** : Writes the changes to disk and exits.

4.  **Format the New Partition**
    Format the newly created partition (`sdX1`) to FAT32:
    ```bash
    sudo mkfs.vfat -F 32 -n "MY_USB" /dev/sdX1
    ```
    *(Note: Ensure you target the partition `sdX1`, not the whole disk `sdX`)*.

5.  **Finished**
    Remove and re-insert the USB drive. It is now ready for normal use.

---

## Additional Notes

* **GUI Alternatives:** If you prefer graphical tools:
    * **Windows:** You can use [Rufus](https://rufus.ie/). Select "Boot selection: Non bootable" to format it back to normal.
    * **Linux:** You can use **GParted**. Select the USB device -> Device -> Create Partition Table -> msdos -> Apply.
    