#!/bin/bash

RAM=4G
IMG="hdd.img"
EXTRA_HDD=""

# Check if hdd1.img exists
if [ -f "hdd1.img" ]; then
    echo "Found hdd1.img, attaching..."
    EXTRA_HDD="-device ahci,id=ahci0 -drive file=hdd1.img,format=raw,if=none,id=drive0 -device ide-hd,drive=drive0,bus=ahci0.0"
fi

# Running project with QEMU and 4GiB of RAM intialized
qemu-system-i386 -m $RAM \
    -drive file="$IMG",format=raw,if=ide,index=0 \
    $EXTRA_HDD \
    -boot d
