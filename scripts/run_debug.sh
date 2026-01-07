#!/bin/bash

RAM=4G
IMG="hdd.img"

# Running QEMU with GDB
qemu-system-i386 -s -S -m $RAM \
  -drive file="$IMG",format=raw,if=ide,index=0 \
  -device ahci,id=ahci0 \
  -drive file=hdd1.img,format=raw,if=none,id=drive0 \
  -device ide-hd,drive=drive0,bus=ahci0.0 \
  -boot d &

# Waiting for QEMU
while ! nc -z localhost 1234; do
  sleep 0.1
done
