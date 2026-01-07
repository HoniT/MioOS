#!/bin/bash

IMG="hdd.img"
IMG1="hdd1.img"

# Create disk image if it doesn't exist (for ATA)
if [ ! -f $IMG ]; then
  echo "Creating ATA disk image ($IMG)..."
  qemu-img create -f raw $IMG 64M
  echo "Creating Ext2 file system in $IMG..."
  mkfs.ext2 -F $IMG
fi

# Create disk image if it doesn't exist (for AHCI)
if [ ! -f $IMG1 ]; then
  echo "Creating ATA disk image ($IMG1)..."
  qemu-img create -f raw $IMG1 64M
  echo "Creating Ext2 file system in $IMG1..."
  mkfs.ext2 -F $IMG1
fi
