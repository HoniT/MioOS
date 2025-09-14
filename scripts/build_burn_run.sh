#!/bin/bash
set -e

# --------------------------
# Paths and settings
# --------------------------
ISO="iso/mio_os.iso"       # ISO file
IMG="hdd.img"              # Output HDD image
SIZE_MB=16                 # HDD size in MB
MNT_ISO="/mnt/iso"
MNT_HDD="/mnt/hdd"

# --------------------------
# Add GCC cross-compiler to PATH
# --------------------------
echo "Adding GCC files to PATH..."
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# --------------------------
# Build kernel and ISO
# --------------------------
echo "Compiling kernel..."
make clean
make all

echo "Making ISO image..."
grub-mkrescue -o "$ISO" iso/

# --------------------------
# Create HDD image
# --------------------------
if [ ! -f hdd.img ]; then
    echo "Creating raw HDD image..."
    dd if=/dev/zero of="$IMG" bs=1M count=$SIZE_MB

    # --------------------------
    # Partition HDD (single bootable primary)
    # --------------------------
    echo "Partitioning $IMG..."
    parted -s "$IMG" mklabel msdos
    parted -s "$IMG" mkpart primary ext2 1MiB 100%
    parted -s "$IMG" set 1 boot on
fi

# --------------------------
# Setup loop device
# --------------------------
LOOP=$(sudo losetup -f --show -P "$IMG")
echo "Loop device: $LOOP"

# --------------------------
# Format partition as ext2
# --------------------------
sudo mkfs.ext2 "${LOOP}p1"

# --------------------------
# Mount partition
# --------------------------
sudo mkdir -p "$MNT_HDD"
sudo mount "${LOOP}p1" "$MNT_HDD"

# --------------------------
# Install GRUB
# --------------------------
sudo grub-install --target=i386-pc --boot-directory="$MNT_HDD/boot" "$LOOP"

# --------------------------
# Mount ISO and copy contents
# --------------------------
sudo mkdir -p "$MNT_ISO"
sudo mount -o loop "$ISO" "$MNT_ISO"
sudo cp -r "$MNT_ISO"/* "$MNT_HDD"/

# --------------------------
# Create hardcoded grub.cfg
# --------------------------
sudo tee "$MNT_HDD/boot/grub/grub.cfg" > /dev/null <<EOF
menuentry "MyOS" {
    multiboot /boot/mio_os.elf
    boot
}
EOF

# --------------------------
# Cleanup
# --------------------------
sudo umount "$MNT_ISO" || true
sudo umount "$MNT_HDD" || true
sudo losetup -d "$LOOP"

echo "Bootable HDD image created: $IMG"

# --------------------------
# Run in QEMU
# --------------------------
sudo qemu-system-i386 -enable-kvm -m 8G -drive file="$IMG",format=raw,if=ide,index=0 -boot d
