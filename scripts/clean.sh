#!/bin/bash
source "$(dirname "$0")/config.sh"

# Cleanup mounts just in case a previous build failed
sudo umount -l /tmp/mnt_hdd 2>/dev/null || true
sudo umount -l /tmp/mnt_iso 2>/dev/null || true
sudo losetup -D 2>/dev/null || true

# Remove generated disk images
rm -f "$MAIN_IMG" "$EXTRA_IMG" "$ISO_FILE"

# Clean the source code binaries
make clean

# Remove log files or other temporary build artifacts if they exist
rm -rf iso/boot/*.elf
rm -rf *.log

echo "Project cleaned."