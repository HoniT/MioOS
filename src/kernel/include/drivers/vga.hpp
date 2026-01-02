// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#ifndef VGA_HPP
#define VGA_HPP

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

extern uint32_t default_rgb_color;

enum VGA_Modes {
    TEXT,
    FRAMEBUFFER
};

struct multiboot_tag_framebuffer;
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
    uint32_t get_pixel(const uint32_t x, const uint32_t y);
} // namespace vga

#endif // VGA_HPP
