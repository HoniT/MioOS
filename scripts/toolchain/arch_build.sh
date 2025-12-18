#!/bin/bash
set -e

# ---------------------------------
# Update System
# ---------------------------------
echo "Updating system..."
sudo pacman -Syu --noconfirm

# ---------------------------------
# Install Dependencies
# ---------------------------------
echo "Installing dependencies..."

DEPENDENCIES=(
    "base-devel"
    "qemu"
    "nasm"
    "gdb"
    "grub"
    "libisoburn"
    "mtools"
    "util-linux"
    "e2fsprogs"
    "parted"
    "gmp"
    "libmpc"
    "mpfr"
    "texinfo"
    "isl"
    "wget"
)

sudo pacman -S --needed --noconfirm "${DEPENDENCIES[@]}"

export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"


if command -v "$TARGET-gcc" &> /dev/null; then
    echo "$TARGET cross-compiler already installed."
else
    echo "Building $TARGET cross-compiler..."

    mkdir -p "$HOME/src"
    cd "$HOME/src"

    BINUTILS_VER=2.42
    GCC_VER=14.1.0

    # ------------------------------
    # Build binutils
    # ------------------------------
    if [ ! -d "binutils-$BINUTILS_VER" ]; then
        wget https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VER.tar.xz
        tar -xf binutils-$BINUTILS_VER.tar.xz
    fi

    mkdir -p build-binutils
    cd build-binutils

    ../binutils-$BINUTILS_VER/configure \
        --target=$TARGET \
        --prefix=$PREFIX \
        --with-sysroot \
        --disable-nls \
        --disable-werror

    make -j$CORES
    make install

    cd ..

    # ------------------------------
    # Build gcc 
    # ------------------------------
    if [ ! -d "gcc-$GCC_VER" ]; then
        wget https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VER/gcc-$GCC_VER.tar.xz
        tar -xf gcc-$GCC_VER.tar.xz
    fi

    mkdir -p build-gcc
    cd build-gcc

    ../gcc-$GCC_VER/configure \
        --target=$TARGET \
        --prefix=$PREFIX \
        --disable-nls \
        --enable-languages=c,c++ \
        --without-headers \
        --disable-hosted-libstdcxx

    echo "Compiling GCC (this will take a while)..."
    make all-gcc
    make all-target-libgcc
    make all-target-libstdc++-v3
    
    make install-gcc
    make install-target-libgcc
    make install-target-libstdc++-v3

    echo "Cross-compiler installed at $PREFIX"
fi

echo "All dependencies and cross-compiler are installed."
echo "Don't forget to add $PREFIX/bin to your PATH in your .bashrc or .zshrc!"
