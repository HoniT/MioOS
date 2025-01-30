
# Adding GCC cross-compiler files to PATH
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Building project
make clean
make all

# Making ISO
grub-mkrescue -o iso/mio_os.iso iso/
