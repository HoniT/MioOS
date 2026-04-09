#!/bin/bash
set -e

# --- Path Resolution ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$([[ "$SCRIPT_DIR" == */scripts ]] && dirname "$SCRIPT_DIR" || echo "$SCRIPT_DIR")"

# --- Configuration ---
ISO_DIR="$REPO_ROOT/iso"
ISO_OUT="$ISO_DIR/mio_os.iso"
DISK_IMG="$REPO_ROOT/data_ide.img"
MNT_HDD="/tmp/mnt_mio_hdd"

export PATH="$HOME/opt/cross/bin:$PATH"

# --- Logic ---

build_os() {
    echo "[*] Compiling changed files..."
    cd "$REPO_ROOT" && make all
    mkdir -p "$(dirname "$ISO_OUT")"
    grub-mkrescue -o "$ISO_OUT" "$ISO_DIR"
    echo "[+] ISO Ready: $ISO_OUT"
}

burn_hdd() {
    echo "[*] Installing OS to $DISK_IMG..."
    [ ! -f "$DISK_IMG" ] && echo "Error: Run './mio.sh disk' first." && exit 1
    
    # 1. Setup loopback
    LOOP=$(sudo losetup -f --show -P "$DISK_IMG")
    PART="${LOOP}p1"
    trap "sudo umount $MNT_HDD 2>/dev/null || true; sudo losetup -d $LOOP 2>/dev/null || true" EXIT

    # 2. Install GRUB & Copy Files
    sudo mkfs.ext2 -q -m 0 -I 128 "$PART"
    mkdir -p "$MNT_HDD"
    sudo mount "$PART" "$MNT_HDD"
    
    sudo grub-install --target=i386-pc --boot-directory="$MNT_HDD/boot" --modules="biosdisk part_msdos ext2" "$LOOP"
    sudo cp -r "$ISO_DIR"/. "$MNT_HDD"/
    sudo cp "$ISO_DIR/boot/grub/grub.cfg" "$MNT_HDD/boot/grub/grub.cfg"
    
    sync
    echo "[+] HDD Successfully Burned!"
}

run_os() {
    local ARGS=(-m "${RAM:-4G}" -serial stdio)
    local BOOT="iso"

    while [[ "$#" -gt 0 ]]; do
        case $1 in
            --kvm)   ARGS+=(-enable-kvm) ;;
            --debug) ARGS+=(-s -S) ;;
            --hdd)   BOOT="hdd" ;;
            --ram)   RAM="$2"; shift ;;
        esac
        shift
    done

    if [ "$BOOT" = "iso" ]; then
        echo "[+] Booting ISO..."
        qemu-system-x86_64 "${ARGS[@]}" -cdrom "$ISO_OUT" -boot d -drive file="$DISK_IMG",format=raw,if=ide,index=1
    else
        echo "[+] Booting HDD..."
        qemu-system-x86_64 "${ARGS[@]}" -drive file="$DISK_IMG",format=raw,if=ide,index=0 -boot c
    fi
}

case "$1" in
    build) build_os ;;
    burn)  burn_hdd ;;
    disk)  qemu-img create -f raw "$DISK_IMG" "${2:-128M}"
           parted -s "$DISK_IMG" mklabel msdos mkpart primary ext2 1MiB 100% set 1 boot on ;;
    run)   shift; run_os "$@" ;;
    clean) cd "$REPO_ROOT" && make clean && rm -f "$DISK_IMG" "$ISO_OUT" ;;
    *)     echo "Usage: ./mio.sh {build|burn|run|disk|clean}"; exit 1 ;;
esac