#!/bin/bash
source "$(dirname "$0")/config.sh"

rm -f "$ISO_FILE"
rm -f "$KERNEL_ELF"
make all

grub-mkrescue -o "$ISO_FILE" iso/

bash "$(dirname "$0")/create_disks.sh"

sudo "$QEMU_CMD" \
  -enable-kvm \
  -m "$RAM_DEV" \
  -drive file="$ISO_FILE",format=raw,if=ide,index=0 \
  -drive file="$MAIN_IMG",format=raw,if=ide,index=1 \
  -device ahci,id=ahci0 \
  -drive file="$EXTRA_IMG",format=raw,if=none,id=drive0 \
  -device ide-hd,drive=drive0,bus=ahci0.0 \
  -boot d