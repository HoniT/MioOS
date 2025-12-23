# Creating a Bootable USB from a `.iso` File

This guide explains how to create a bootable USB drive from a `.iso` file on both Linux and Windows systems.

---

## Prerequisites

1.  **Build the Project:**
    Run the build script to generate the system image:
    ```bash
    bash ./scripts/build.sh
    ```
    This will generate the bootable image at `iso/mio_os.iso` (relative to the project root).
2.  **USB Drive:** A USB flash drive (Note: **This process will erase all data on it**).
3.  **Permissions:** Administrative privileges (`sudo` on Linux, Administrator on Windows).

---

## Warnings

* **Data Destruction:** This process will completely **erase all data** on the target USB drive. Back up any important files before proceeding.
* **Drive Identification:** Be extremely careful to identify the correct drive letter/path (e.g., `/dev/sdX`). Writing to the wrong drive (like your hard disk) will destroy your operating system.

---

## Instructions

### **Linux**

1.  **Insert the USB Drive**
    Plug your USB drive into your computer.

2.  **Identify the USB Drive**
    Open a terminal and run:
    ```bash
    lsblk
    ```
    Look for your USB drive in the list. It usually appears as `/dev/sdb`, `/dev/sdc`, etc.
    * *Note: Do not use `/dev/sda` as that is typically your main hard drive.*

3.  **Unmount the USB Drive**
    Unmount any auto-mounted partitions (where `X` is your drive letter):
    ```bash
    sudo umount /dev/sdX*
    ```

4.  **Write the `.iso` File**
    Use the `dd` command to write the image bit-by-bit.
    * Replace `path/to/mio_os.iso` with the actual path to your ISO.
    * Replace `/dev/sdX` with your USB device identifier (e.g., `/dev/sdb`).
    ```bash
    sudo dd if=path/to/mio_os.iso of=/dev/sdX bs=4M status=progress
    ```

5.  **Sync and Eject**
    Ensure all write operations are flushed to the disk before removing:
    ```bash
    sudo sync
    ```
    You may now remove the USB drive.

---

### **Windows**

1.  **Insert the USB Drive**
    Plug your USB drive into your computer.

2.  **Download and Install Rufus**
    Download the latest version of [Rufus](https://rufus.ie/) and open it.

3.  **Select the USB Drive**
    In Rufus, ensure your USB drive is selected in the **"Device"** dropdown.

4.  **Choose the `.iso` File**
    Under **"Boot selection"**, click **"SELECT"** and navigate to your `mio_os.iso` file.

5.  **Start Writing**
    * Click **"START"**.

6.  **Confirm Erase**
    Confirm the warning about erasing all data on the USB.

7.  **Safely Eject**
    Once the status bar says "READY", close Rufus and safely eject the USB drive.

---

## Testing the Bootable USB

1.  **Enter BIOS/UEFI**
    Restart your computer and enter the BIOS setup (usually `Del`, `F2`, or `F12` during startup).

2.  **Configure Boot Settings**
    Because this is a custom OS, you likely need to adjust these settings:
    * **Secure Boot:** Set to **Disabled**.
    * **Boot Mode:** Enable **Legacy** or **CSM (Compatibility Support Module)**. Most hobby OS kernels are not UEFI signed and require Legacy BIOS emulation.

3.  **Boot**
    Set the USB drive as the first priority in the "Boot Order" menu, save changes, and restart (or just open in boot menu).

---

## Troubleshooting

* **"Operating System Not Found" / Skipped Boot:**
    * Did you enable **CSM/Legacy Boot** in BIOS?
    * Did you disable **Secure Boot**?

* **USB Not Recognized in Linux:**
    Double-check the identifier with `lsblk` or `dmesg | tail` after plugging it in.

* **Windows Cannot Read USB After Use:**
    Windows may complain that the drive is corrupt or unreadable after using it for a custom OS. This is normal. To restore the drive for regular storage use, refer to the [Reinitializing USB](ReinitializingUSB.md) guide.

---

## Additional Resources

- [Rufus Documentation](https://rufus.ie/)
- [Linux dd Command Guide](https://linux.die.net/man/1/dd)
