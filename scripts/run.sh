#!/bin/bash

RAM=4G
IMG="hdd.img"

# Running project with QEMU and 4GiB of RAM intialized
qemu-system-i386 -m $RAM -drive file="$IMG",format=raw,if=ide,index=0 -boot d
