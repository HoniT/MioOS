#!/bin/bash

MAIN_IMG="hdd_main.img"
EXTRA_IMG="hdd_extra.img"

# Create disk image if it doesn't exist (for ATA)
if [ ! -f $MAIN_IMG ]; then
  echo "Creating disk image ($MAIN_IMG)..."
  qemu-img create -f raw $MAIN_IMG 64M
  echo "Creating Ext2 file system in $MAIN_IMG..."
  mkfs.ext2 -F $MAIN_IMG
fi

# Create disk image if it doesn't exist (for AHCI)
if [ ! -f $EXTRA_IMG ]; then
  echo "Creating disk image ($EXTRA_IMG)..."
  qemu-img create -f raw $EXTRA_IMG 64M
  echo "Creating Ext2 file system in $EXTRA_IMG..."
  mkfs.ext2 -F $EXTRA_IMG
fi
