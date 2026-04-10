#!/bin/bash
set -e
source "$(dirname "$0")/config.sh"

MNT_ISO="/tmp/mnt_iso"
MNT_HDD="/tmp/mnt_hdd"

make clean
make all

grub-mkrescue -o "$ISO_FILE" iso/

sudo umount -l "$MNT_HDD" 2>/dev/null || true
sudo umount -l "$MNT_ISO" 2>/dev/null || true
sudo losetup -D
rm -f "$MAIN_IMG"

dd if=/dev/zero of="$MAIN_IMG" bs=1M count="$MAIN_IMG_SIZE_MB"
parted -s "$MAIN_IMG" mklabel msdos
parted -s "$MAIN_IMG" mkpart primary ext2 1MiB 100%
parted -s "$MAIN_IMG" set 1 boot on

LOOP=$(sudo losetup -f --show -P "$MAIN_IMG")
PART="${LOOP}p1"

sudo partprobe "$LOOP"
sleep 1 

sudo mkfs.ext2 -m 0 -I 128 "$PART"

mkdir -p "$MNT_HDD"
sudo mount "$PART" "$MNT_HDD"

sudo grub-install --target=i386-pc --boot-directory="$MNT_HDD/boot" "$LOOP"

mkdir -p "$MNT_ISO"
sudo mount -o loop "$ISO_FILE" "$MNT_ISO"
sudo cp -rv "$MNT_ISO"/* "$MNT_HDD"/

sudo cp "$PWD/iso/boot/grub/grub.cfg" "$MNT_HDD/boot/grub/grub.cfg"

sync

sudo umount "$MNT_ISO"
sudo umount "$MNT_HDD"
sudo losetup -d "$LOOP"