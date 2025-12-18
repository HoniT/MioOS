#!/bin/bash

RAM=4G
IMG="hdd.img"

QEMU="qemu-system-i386"
COMMON_OPTS="-m $RAM -drive file=$IMG,format=raw,if=ide,index=0 -boot d"

echo "Checking KVM support..."

# If KVM is supported we'll run with KVM
if egrep -c '(vmx|svm)' /proc/cpuinfo; then
    echo "KVM available - using hardware acceleration"
    sudo $QEMU -enable-kvm $COMMON_OPTS
else
    echo "KVM not available - falling back to software emulation"
    $QEMU $COMMON_OPTS
fi
