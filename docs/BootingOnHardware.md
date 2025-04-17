# Creating a Bootable USB from a `.iso` File

This guide explains how to create a bootable USB drive from a `.iso` file on both Linux and Windows systems.

---

## Prerequisites

1. Build this project with 
    ``` bash
    bash ./scripts/build.sh
    ```
   and follow the steps bellow using the BIN file in the bin directory (path/to/this/project/iso/mio_os.iso).
2. A USB drive (this process will erase all data on it).
3. Administrative privileges on your computer.

---

## Warnings
- **Back up your USB drive**: This process will completely erase all data on the USB drive.
- **Correct USB identification**: Ensure you correctly identify the USB drive to avoid overwriting the wrong disk.

---

## Instructions

### **Linux**

1. **Insert the USB Drive**
   - Plug your USB drive into your computer.

2. **Identify the USB Drive**
   - Open a terminal and run:
     ```bash
     lsblk
     ```
   - Look for your USB drive in the list (e.g., `/dev/sdX`, where `X` is the drive letter).

3. **Unmount the USB Drive**
   - Unmount any mounted partitions of the USB drive:
     ```bash
     sudo umount /dev/sdX*
     ```

4. **Write the `.iso` File to the USB Drive**
   - Use the `dd` command to copy the `.iso` file to the USB drive:
     ```bash
     sudo dd if=path/to/your_file.iso of=/dev/sdX bs=4M status=progress
     ```
     Replace `path/to/your_file.iso` with the path to your `.iso` file and `/dev/sdX` with your USB drive.

5. **Sync and Eject**
   - Ensure all data is written to the USB drive:
     ```bash
     sudo sync
     ```
   - Remove the USB drive safely.

---

### **Windows**

1. **Insert the USB Drive**
   - Plug your USB drive into your computer.

2. **Download and Install Rufus**
   - Download [Rufus](https://rufus.ie/).
   - Install and open Rufus.

3. **Select the USB Drive**
   - In Rufus, select your USB drive from the "Device" dropdown.

4. **Choose the `.iso` File**
   - Under "Boot selection," click "SELECT" and choose your `.iso` file.

5. **Set Partition Scheme**
   - Select `MBR` as the partition scheme.
   - Choose the appropriate target system (usually `BIOS`).

6. **Start Writing**
   - Click "START" to write the `.iso` file to the USB drive.
   - Confirm any warnings about erasing data.

7. **Safely Eject the USB**
   - Once the process is complete, eject the USB drive safely.

---

## Testing the Bootable USB

1. **Set USB as the Boot Device**
   - Restart your computer and enter the BIOS/UEFI settings (usually by pressing `Del`, `F2`, or `F12` during startup).
   - Set the USB drive as the first boot device.

2. **Boot the System**
   - Save the BIOS/UEFI settings and restart.
   - Your computer should now boot into the OS from the USB drive.

---

## Troubleshooting

- **USB Not Recognized**: Double-check the device name (`/dev/sdX` on Linux) or the selected drive in Rufus.
- **Boot Failure**: Ensure the `.iso` file is correctly built and compatible with your hardware.
- **File Explorer Cannot See the USB**: This is normal if the USB contains a raw binary image. Reformat the USB to restore it for normal use, check out [Reinitializing USB](ReinitializingUSB.md);

---

## Additional Resources

- [Rufus Documentation](https://rufus.ie/)
- [Linux dd Command Guide](https://linux.die.net/man/1/dd)

