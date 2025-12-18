#!/bin/bash
set -e

# ---------------------------------
# Package install helper
# ---------------------------------
check_and_install() {
    PACKAGE=$1
    if dpkg -s "$PACKAGE" &> /dev/null; then
        echo "$PACKAGE is already installed."
    else
        echo "$PACKAGE is not installed. Installing..."
        sudo apt-get install -y "$PACKAGE"
    fi
}

# ---------------------------------
# Update package list
# ---------------------------------
echo "Updating package list"
sudo apt-get update

# ---------------------------------
# System packages
# ---------------------------------
check_and_install "make"
check_and_install "qemu-system-i386"
check_and_install "nasm"
check_and_install "gcc"
check_and_install "g++"
check_and_install "gdb"
check_and_install "binutils"
check_and_install "grub-common"
check_and_install "grub-pc-bin"
check_and_install "xorriso"
check_and_install "mtools"
check_and_install "util-linux"
check_and_install "e2fsprogs"
check_and_install "parted"

# Cross-compiler build deps
check_and_install "build-essential"
check_and_install "bison"
check_and_install "flex"
check_and_install "libgmp3-dev"
check_and_install "libmpc-dev"
check_and_install "libmpfr-dev"
check_and_install "texinfo"
check_and_install "libisl-dev"

# ---------------------------------
# Build i686-elf cross compiler
# ---------------------------------
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

if command -v "$TARGET-gcc" &> /dev/null; then
    echo "$TARGET cross-compiler already installed."
else
    echo "Building $TARGET cross-compiler..."

    mkdir -p "$HOME/src"
    cd "$HOME/src"

    # Versions (known-good)
    BINUTILS_VER=2.42
    GCC_VER=14.1.0

    # ------------------------------
    # Build binutils
    # ------------------------------
    wget https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VER.tar.xz
    tar -xf binutils-$BINUTILS_VER.tar.xz
    mkdir -p build-binutils
    cd build-binutils

    ../binutils-$BINUTILS_VER/configure \
        --target=$TARGET \
        --prefix=$PREFIX \
        --with-sysroot \
        --disable-nls \
        --disable-werror

    make
    make install

    cd ..

    # ------------------------------
    # Build gcc 
    # ------------------------------
    wget https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VER/gcc-$GCC_VER.tar.xz
    tar -xf gcc-$GCC_VER.tar.xz
    mkdir -p build-gcc
    cd build-gcc

    ../gcc-$GCC_VER/configure \
        --target=$TARGET \
        --prefix=$PREFIX \
        --disable-nls \
        --enable-languages=c,c++ \
        --without-headers \
        --disable-hosted-libstdcxx

    make all-gcc
    make all-target-libgcc
    make all-target-libstdc++-v3
    make install-gcc
    make install-target-libgcc
    make install-target-libstdc++-v3

    echo "Cross-compiler installed at $PREFIX"
fi

echo "All dependencies and cross-compiler are installed."
