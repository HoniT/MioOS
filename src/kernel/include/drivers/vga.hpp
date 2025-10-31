// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#ifndef VGA_HPP
#define VGA_HPP

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <kernel_main.hpp>

// The 16 standard VGA colors in RGB values

#define RGB_COLOR_BLACK        0x000000
#define RGB_COLOR_BLUE         0x0000AA
#define RGB_COLOR_GREEN        0x00AA00
#define RGB_COLOR_CYAN         0x00AAAA
#define RGB_COLOR_RED          0xAA0000
#define RGB_COLOR_MAGENTA      0xAA00AA
#define RGB_COLOR_BROWN        0xAA5500
#define RGB_COLOR_LIGHT_GRAY   0xAAAAAA
#define RGB_COLOR_DARK_GRAY    0x555555
#define RGB_COLOR_LIGHT_BLUE   0x5555FF
#define RGB_COLOR_LIGHT_GREEN  0x55FF55
#define RGB_COLOR_LIGHT_CYAN   0x55FFFF
#define RGB_COLOR_LIGHT_RED    0xFF5555
#define RGB_COLOR_PINK         0xFF55FF
#define RGB_COLOR_YELLOW       0xFFFF55
#define RGB_COLOR_WHITE        0xFFFFFF

extern uint32_t default_rgb_color;

enum VGA_Modes {
    TEXT,
    FRAMEBUFFER
};

namespace vga {
    extern uint32_t* framebuffer;
    extern uint32_t fb_size;

    extern uint32_t screen_width;
    extern uint32_t screen_height;
    extern uint32_t screen_pitch;
    extern uint8_t screen_bpp; // Bits per pixel
    
    extern uint8_t font_height;
    extern uint8_t font_width;
    extern uint32_t screen_col_num;
    extern uint32_t screen_row_num;

    VGA_Modes get_vga_mode(void);
    
    // Initialization
    void init_framebuffer(const multiboot_tag_framebuffer* fb_tag);

    // Bare metal output functions
    void put_pixel(const uint32_t x, const uint32_t y, const uint32_t color);
} // namespace vga

#endif // VGA_HPP
