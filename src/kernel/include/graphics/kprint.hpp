// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef KPRINT_HPP
#define KPRINT_HPP

#include <mm/mm_defs.hpp>
#include <stdarg.h>

using mem::PhysAddr;

/// @brief VGA text mode logic
namespace vga
{
    // We won't support advanced instructions for text mode. Just the basics

    constexpr PhysAddr VGA_TEXT_ADDRESS = 0xB8000 + mem::HIGHER_HALF_OFFSET;
    constexpr PhysAddr NUM_COLS = 80;
    constexpr PhysAddr NUM_ROWS = 25;
    constexpr uint8_t VGA_TEXT_DEFAULT_COLOR = 15 | (0 << 4);

    /// @brief VGA text mode formatted print logic
    /// @param fmt Text format
    /// @param args Print parameters
    void kvprintf(const char* fmt, va_list args);

    /// @brief Clears screen
    void kclear();
} // namespace vga

/// @brief Print function definition. Will call necessary implementation to print
/// @param fmt Text format
/// @param ... Print parameters
void kprintf(const char* fmt, ...);
/// @brief Same as printf but print arguments are passed with variadic lists
void kvprintf(const char* fmt, va_list args);
// Will add more printf ovverides and othe functions in future

#endif // KPRINT_HPP
