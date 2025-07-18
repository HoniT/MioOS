
# Adding GCC cross-compiler files to PATH
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Building project
make clean
make all

# Making ISO
grub-mkrescue -o iso/mio_os.iso iso/

# Create ATA disk image if it doesn't exist (16MB)
if [ ! -f hdd.img ]; then
  echo "Creating ATA disk image (hdd.img)..."
  qemu-img create -f raw hdd.img 16M
  mkfs.ext2 -F hdd.img
fi
