// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VGA_PRINT_HPP
#define VGA_PRINT_HPP

#define VGA_ADDRESS 0xB8000 // VGA address
#define SCROLLBACK_MAX_LINES 100 // Maximum amount of lines that we will store

// Size constraints
#define NUM_COLS 80
#define NUM_ROWS 25

// Default colors
#define DEFAULT_FG_COLOR PRINT_COLOR_WHITE
#define DEFAULT_BG_COLOR PRINT_COLOR_BLACK

#include <stdint.h>
#include <stddef.h>

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

// Current coordinates/address
extern size_t col;
extern size_t row;

struct vga_coords {
	size_t col;
	size_t row;
};

namespace vga {

	void init(void); // Initializes main VGA text

	vga_coords set_init_text(const char* text);
	void set_init_text_answer(vga_coords coords, bool passed);

	// Helper functions

	void clear_region(const size_t _row, const size_t _col, const uint32_t len);
	void insert(size_t _row, size_t _col, const char* str, bool _update_cursor = false, uint8_t color = DEFAULT_FG_COLOR | (DEFAULT_BG_COLOR << 4));

	// VGA printing functions

	// Print formatted
	vga_coords printf(const char* format, ...);

	// Error messages
	vga_coords error(const char* format, ...);
	// Warnings
	vga_coords warning(const char* format, ...);

	void print_clear(void); // Clearing screen
	void backspace(void);

	void print_set_color(const uint8_t foreground, const uint8_t background); // Changing text color

} // Namespace vga

#endif // VGA_PRINT_HPP
