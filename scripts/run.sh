#!/bin/bash

RAM=4G
MAIN_IMG="hdd_main.img"
EXTRA_IMG="hdd_extra.img"
EXTRA_HDD=""

# Check if hdd_extra.img exists
if [ -f $EXTRA_IMG ]; then
    echo "Found $EXTRA_IMG, attaching..."
    EXTRA_HDD="-device ahci,id=ahci0 -drive file=$EXTRA_IMG,format=raw,if=none,id=drive0 -device ide-hd,drive=drive0,bus=ahci0.0"
fi

# Running project with QEMU and 4GiB of RAM intialized
qemu-system-i386 -m $RAM \
    -drive file="$IMG",format=raw,if=ide,index=0 \
    $EXTRA_HDD \
    -boot d
