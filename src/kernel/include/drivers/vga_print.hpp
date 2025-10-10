// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VGA_PRINT_HPP
#define VGA_PRINT_HPP

#pragma region VGA Text Mode

#define VGA_ADDRESS 0xB8000 // VGA address

// Size constraints
#define NUM_COLS 80
#define NUM_ROWS 25

// VGA text mode default color (white on black)
#define VGAT_COLOR 15 | (0 << 4)

#pragma endregion

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

enum PrintTypes {
    STD_PRINT,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

struct vga_coords {
	size_t col;
	size_t row;
};

namespace vga {
    extern int col_num;
    extern int row_num;

    // Screen features
    void clear_text_region(const size_t col, const size_t row, const size_t len);
    void backspace(void);
    void clear_screen(void);
    void insert(const size_t col, const size_t row, const uint32_t color, const bool update_pos, const char* fmt, ...);
    void insert(const size_t col, const size_t row, const bool update_pos, const char* fmt, ...);

    void vgat_vprintf(const char* format, va_list args);
} // namespace vga


// Print functions
void kputchar(const uint32_t color, const char c);
void kputs(const uint32_t color, const char* str);
vga_coords kprintf(const char* fmt, ...);
vga_coords kprintf(const uint32_t color, const char* fmt, ...);
vga_coords kprintf(const PrintTypes print_type, const char* fmt, ...);
vga_coords kprintf(const PrintTypes print_type, const uint32_t color, const char* fmt, ...);
void kvprintf(const PrintTypes print_type, const uint32_t color, const char* fmt, const va_list args);

vga_coords kvprintf_at(const size_t col, const size_t row, const PrintTypes print_type, const uint32_t color, const bool update_pos, const char* fmt, va_list args);

#endif // VGA_PRINT_HPP
