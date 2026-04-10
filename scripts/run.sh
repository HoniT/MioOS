#!/bin/bash
source "$(dirname "$0")/config.sh"

EXTRA_HDD=""

if [ -f "$EXTRA_IMG" ]; then
    EXTRA_HDD="-device ahci,id=ahci0 -drive file=$EXTRA_IMG,format=raw,if=none,id=drive0 -device ide-hd,drive=drive0,bus=ahci0.0"
fi

"$QEMU_CMD" -m "$RAM_DEFAULT" \
    -drive file="$MAIN_IMG",format=raw,if=ide,index=0 \
    $EXTRA_HDD \
    -boot d