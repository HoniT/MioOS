#!/bin/bash
set -e

# Settings
ISO="iso/mio_os.iso"
IMG="hdd.img"
SIZE_MB=128
MNT_ISO="/tmp/mnt_iso"
MNT_HDD="/tmp/mnt_hdd"

# 1. Compiling kernel

# Adding GCC cross-compiler files to PATH
echo "Adding GCC files to PATH..."
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Building project
echo "Compiling kernel..."
make clean
make all

# Making ISO
echo "Making ISO image..."
grub-mkrescue -o iso/mio_os.iso iso/

# 2. Cleaning previous runs
echo "Cleaning up old mounts..."
sudo umount -l "$MNT_HDD" 2>/dev/null || true
sudo umount -l "$MNT_ISO" 2>/dev/null || true
sudo losetup -D  # Detach ALL loop devices to be safe
rm -f "$IMG"

# 3. Create image
echo "Creating $SIZE_MB MB HDD image..."
dd if=/dev/zero of="$IMG" bs=1M count=$SIZE_MB
parted -s "$IMG" mklabel msdos
parted -s "$IMG" mkpart primary ext2 1MiB 100%
parted -s "$IMG" set 1 boot on

# 4. Setup loop
LOOP=$(sudo losetup -f --show -P "$IMG")
PART="${LOOP}p1"

# Wait for kernel to find partitions
sudo partprobe "$LOOP"
sleep 1 

# 5. Format Ext2
echo "Formatting $PART..."
sudo mkfs.ext2 -m 0 -I 128 "$PART"

# 6. Mount & copy
mkdir -p "$MNT_HDD"
sudo mount "$PART" "$MNT_HDD"

echo "Installing GRUB..."
sudo grub-install --target=i386-pc --boot-directory="$MNT_HDD/boot" "$LOOP"

echo "Copying ISO contents..."
mkdir -p "$MNT_ISO"
sudo mount -o loop "$ISO" "$MNT_ISO"
sudo cp -rv "$MNT_ISO"/* "$MNT_HDD"/

# 7. Config
sudo cp "$PWD/iso/boot/grub/grub.cfg" "$MNT_HDD/boot/grub/grub.cfg"

# Sync ensures all data is actually written to the .img file
sync

echo "Cleaning up..."
sudo umount "$MNT_ISO"
sudo umount "$MNT_HDD"
sudo losetup -d "$LOOP"

echo "Done building OS!"
