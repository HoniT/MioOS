#!/bin/bash

export ARCH="x86_64"
export TARGET="${ARCH}-elf"
export QEMU_CMD="qemu-system-${ARCH}"

export PREFIX="$HOME/opt/cross"
export PATH="$PREFIX/bin:$PATH"

export ISO_FILE="iso/mio_os.iso"
export KERNEL_ELF="iso/boot/mio_os.elf"
export MAIN_IMG="hdd_main.img"
export EXTRA_IMG="hdd_extra.img"

export MAIN_IMG_SIZE_MB=128
export DISK_IMG_SIZE="64M"

export RAM_DEFAULT="4G"
export RAM_DEV="8G"