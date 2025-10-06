// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VGA_PRINT_HPP
#define VGA_PRINT_HPP

#define VGA_ADDRESS 0xB8000 // VGA address

// Size constraints
#define NUM_COLS 80
#define NUM_ROWS 25

// Default colors
#define DEFAULT_FG_COLOR PRINT_COLOR_WHITE
#define DEFAULT_BG_COLOR PRINT_COLOR_BLACK

#include <stdint.h>
#include <stddef.h>
#include <drivers/vga.hpp>

// 16 available colors with VGA
enum VGA_PrintColors {

    PRINT_COLOR_BLACK = 0,
	PRINT_COLOR_BLUE = 1,
	PRINT_COLOR_GREEN = 2,
	PRINT_COLOR_CYAN = 3,
	PRINT_COLOR_RED = 4,
	PRINT_COLOR_MAGENTA = 5,
	PRINT_COLOR_BROWN = 6,
	PRINT_COLOR_LIGHT_GRAY = 7,
	PRINT_COLOR_DARK_GRAY = 8,
	PRINT_COLOR_LIGHT_BLUE = 9,
	PRINT_COLOR_LIGHT_GREEN = 10,
	PRINT_COLOR_LIGHT_CYAN = 11,
	PRINT_COLOR_LIGHT_RED = 12,
	PRINT_COLOR_PINK = 13,
	PRINT_COLOR_YELLOW = 14,
	PRINT_COLOR_WHITE = 15

};

vga_coords printf(const char* fmt, ...);
vga_coords printf(const uint32_t color, const char* fmt, ...);
vga_coords printf(const PrintTypes print_type, const char* fmt, ...);
vga_coords printf(const PrintTypes print_type, const uint32_t color, const char* fmt, ...);

namespace vga {
	// Current coordinates/address
	extern size_t col;
	extern size_t row;

	void init(void); // Initializes main VGA text

	// Helper functions

	void clear_region(const size_t _row, const size_t _col, const uint32_t len);
	void insert(size_t _row, size_t _col, const char* str, bool _update_cursor = false, uint8_t color = DEFAULT_FG_COLOR | (DEFAULT_BG_COLOR << 4));

	// VGA printing functions

	// Print formatted
	vga_coords primitive_printf(const char* format, ...);
	vga_coords primitive_printf(const uint8_t color, const char* format, ...);
	vga_coords primitive_vprintf(const char* format, va_list args);

	// Error messages
	vga_coords primitive_error(const char* format, ...);
	vga_coords primitive_verror(const char* format, va_list args);
	// Warnings
	vga_coords primitive_warning(const char* format, ...);
	vga_coords primitive_vwarning(const char* format, va_list args);

	void print_clear(void); // Clearing screen
	void backspace(void);

	void print_set_color(const uint8_t foreground, const uint8_t background); // Changing text color

} // Namespace vga

#endif // VGA_PRINT_HPP
