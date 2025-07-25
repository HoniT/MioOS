
# Running project with QEMU and 8GiB of RAM intialized
qemu-system-i386 -m 8G -drive file=iso/mio_os.iso,format=raw,if=ide,index=0 -drive file=hdd.img,format=raw,if=ide,index=1 -boot d