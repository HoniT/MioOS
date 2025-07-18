
# Running QEMU with gdb
qemu-system-i386 -s -S -m 8G -drive file=iso/mio_os.iso,format=raw,if=ide,index=0 -drive file=hdd.img,format=raw,if=ide,index=1 -boot d &

# Waiting for QEMU
while ! nc -z localhost 1234; do
  sleep 0.1
done