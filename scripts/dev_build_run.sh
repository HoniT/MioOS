#!/bin/bash

# Regular building requires cleaning old output files and reburning ISO to hdd.img.
# Dev build runs the project directly from the ISO.
# While building, the Makefile can only compile changed files to reduce development time.

# Adding GCC cross-compiler files to PATH
echo "Adding GCC files to PATH..."
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Building project
echo "Compiling kernel..."
rm -f iso/mio_os.iso
rm -f iso/boot/mio_os.elf
make all

# Making ISO
echo "Making ISO image..."
grub-mkrescue -o iso/mio_os.iso iso/

# Running project with QEMU (KVM) and 8GiB of RAM intialized
sudo qemu-system-i386 -enable-kvm -m 8G -drive file=iso/mio_os.iso,format=raw,if=ide,index=0 \
    -drive file=hdd.img,format=raw,if=ide,index=1 -boot d
