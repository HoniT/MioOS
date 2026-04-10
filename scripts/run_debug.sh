#!/bin/bash
source "$(dirname "$0")/config.sh"

"$QEMU_CMD" -s -S -m "$RAM_DEFAULT" \
  -drive file="$MAIN_IMG",format=raw,if=ide,index=0 \
  -device ahci,id=ahci0 \
  -drive file="$EXTRA_IMG",format=raw,if=none,id=drive0 \
  -device ide-hd,drive=drive0,bus=ahci0.0 \
  -boot d &

while ! nc -z localhost 1234; do
  sleep 0.1
done