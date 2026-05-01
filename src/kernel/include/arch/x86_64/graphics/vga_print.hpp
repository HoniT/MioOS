// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VGA_PRINT_HPP
#define VGA_PRINT_HPP

#define VGA_TEXT_ADDRESS 0xFFFFFFFF800B8000 // VGA address

// VGA text mode default color (white on black)
#define VGA_TEXT_DEFAULT_COLOR 15 | (0 << 4)

// Size constraints
#define NUM_COLS 80
#define NUM_ROWS 25

/// @brief Meant for legacy VGA text mode output
namespace x86_64::vga
{
    void printf(const char* fmt, ...);
} // namespace x86_64::vga


#endif // VGA_PRINT_HPP