
# Running project with QEMU with KVM virtualization and 8GiB of RAM intialized
sudo qemu-system-i386 -enable-kvm -m 8G -drive file=iso/mio_os.iso,format=raw