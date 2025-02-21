// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VGA_PRINT_HPP
#define VGA_PRINT_HPP

#define VGA_ADDRESS 0xB8000 // VGA address
#define SCROLLBACK_MAX_LINES 100 // Maximum amount of lines that we will store

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

namespace vga {

void init(void); // Initializes main VGA text

// VGA printing functions

// Print formatted
void printf(const char* format, ...);

void print_decimal(uint32_t num);
void print_decimal(uint64_t num);

// Error messages
void error(const char* format, ...);

void print_clear(void); // Clearing screen
void backspace(void);

void print_set_color(const uint8_t foreground, const uint8_t background); // Changing text color

} // Namespace vga

#endif // VGA_PRINT_HPP
