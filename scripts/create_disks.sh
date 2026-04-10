#!/bin/bash
source "$(dirname "$0")/config.sh"

if [ ! -f "$MAIN_IMG" ]; then
  qemu-img create -f raw "$MAIN_IMG" "$DISK_IMG_SIZE"
  mkfs.ext2 -F "$MAIN_IMG"
fi

if [ ! -f "$EXTRA_IMG" ]; then
  qemu-img create -f raw "$EXTRA_IMG" "$DISK_IMG_SIZE"
  mkfs.ext2 -F "$EXTRA_IMG"
fi