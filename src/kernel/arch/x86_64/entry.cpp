// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <arch/x86_64/entry.hpp>
#include <arch/x86_64/multiboot.hpp>
#include <cpu.hpp>
#include <arch/x86_64/mm/pmm_setup.hpp>
#include <arch/x86_64/mm/vmm_setup.hpp>
#include <kernel_main.hpp>
#include <arch/x86_64/graphics/vga_print.hpp>

#include <stdint.h>

static int vga_index = 0;

void klog(const char* msg) noexcept {
    uint16_t* vga_buffer = (uint16_t*)0xFFFFFFFF800B8000;
    
    for (int i = 0; msg[i] != '\0'; ++i) {
        if (vga_index >= 80 * 25) {
            vga_index = 0; 
        }
        
        if (msg[i] == '\n') {
            vga_index = (vga_index / 80 + 1) * 80;
        } else {
            vga_buffer[vga_index++] = (uint16_t)msg[i] | ((uint16_t)15 << 8); 
        }
    }
}

// Prints a signed 64-bit decimal number
void klog_num(int64_t num) noexcept {
    if (num == 0) {
        klog("0");
        return;
    }

    char buffer[32];
    int i = 30; // Start near the end of the buffer
    buffer[31] = '\0'; // Null-terminate
    
    bool is_negative = num < 0;
    // Cast to unsigned to handle INT64_MIN safely and for standard modulo math
    uint64_t unum = is_negative ? (uint64_t)-num : (uint64_t)num;

    // Extract digits right-to-left
    while (unum > 0) {
        buffer[i--] = '0' + (unum % 10);
        unum /= 10;
    }

    if (is_negative) {
        buffer[i--] = '-';
    }

    // Print starting from the first populated character
    klog(&buffer[i + 1]);
}

// Prints an unsigned 64-bit hexadecimal number with a "0x" prefix
void klog_hex(uint64_t num) noexcept {
    if (num == 0) {
        klog("0x0");
        return;
    }

    char buffer[32];
    int i = 30;
    buffer[31] = '\0';

    const char* hex_chars = "0123456789ABCDEF";

    while (num > 0) {
        buffer[i--] = hex_chars[num % 16];
        num /= 16;
    }

    // Add prefix
    buffer[i--] = 'x';
    buffer[i--] = '0';

    klog(&buffer[i + 1]);
}

[[noreturn]]  void kpanic(const char* msg) noexcept {
    klog("\nPANIC: ");
    klog(msg);
    asm volatile("cli");
    for (;;) asm volatile("hlt");
}

/// @brief Entry point for x86_64 arch
/// @param mbi Multiboot2 info structure
/// @param magic GRUB magic
extern "C" void entry_x86_64(void* mbi, uint32_t magic) {
    // Managing GRUB multiboot error
    if(magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        hal::cpu::halt();
        return;
    }
    
    // Memory manager init
    x86_64::mem::init_pmm(mbi);
    x86_64::mem::vmm_init();

    x86_64::vga::printf("Successfully booted into a %d-bit Higher half %s\n", 64, "kernel");

    // Calling arch independent main function
    kernel_main();
}