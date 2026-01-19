#!/bin/bash

RAM=4G
IMG="hdd_main.img"
EXTRA_IMG="hdd_extra.img"
EXTRA_HDD=""

echo "Checking KVM support..."

# If KVM is supported we'll run with KVM
if egrep -c '(vmx|svm)' /proc/cpuinfo; then
    echo "KVM available - using hardware acceleration"

    # Check if hdd1.img exists
    if [ -f $EXTRA_IMG ]; then
        echo "Found $EXTRA_IMG, attaching..."
        EXTRA_HDD="-device ahci,id=ahci0 -drive file=$EXTRA_IMG,format=raw,if=none,id=drive0 -device ide-hd,drive=drive0,bus=ahci0.0"
    fi

    sudo qemu-system-i386 -enable-kvm \
    -m $RAM -drive file=$IMG,format=raw,if=ide,index=0 \
    $EXTRA_HDD \
    -boot d
else
    echo "KVM not available - falling back to software emulation"
    bash ./scripts/run.sh
fi
