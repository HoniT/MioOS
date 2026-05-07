#!/bin/bash

export TARGET="x86_64-elf"
export QEMU_CMD="qemu-system-x86_64"
export PREFIX="$HOME/opt/cross"
export PATH="$PREFIX/bin:$PATH"

export ISO_FILE="iso/mio_os.iso"
export KERNEL_ELF="iso/boot/mio_os.elf"
export MAIN_IMG="env/hdd_main.img"
export EXTRA_IMG="env/hdd_extra.img"

export MAIN_IMG_SIZE_MB=128

# Emulation settings (change as you wish)

export DISK_IMG_SIZE="64M"

export RAM_DEFAULT="4G"
export RAM_DEV="8G"
