#!/bin/bash

IMG="hdd.img"

# Create ATA disk image if it doesn't exist
if [ ! -f $IMG ]; then
  echo "Creating ATA disk image ($IMG)..."
  qemu-img create -f raw $IMG 128M
  echo "Creating Ext2 file system in $IMG..."
  mkfs.ext2 -F $IMG
fi
