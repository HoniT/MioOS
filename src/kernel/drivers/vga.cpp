// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// vga.cpp
// Sets up a driver for VGA framebuffer drawing and printf
// ========================================

#include <drivers/vga.hpp>
#include <graphics/vga_print.hpp>
#include <multiboot.hpp>
#include <lib/math.hpp>
#include <mm/pmm.hpp>
#include <x86/interrupts/kernel_panic.hpp>

uint32_t* vga::framebuffer = nullptr;
uint32_t* vga::backbuffer = nullptr;
uint32_t vga::fb_size;
uint32_t vga::screen_width;
uint32_t vga::screen_height;
uint32_t vga::screen_pitch;
uint8_t vga::screen_bpp; // Bits per pixel

uint8_t vga::font_height;
uint8_t vga::font_width;
uint32_t vga::screen_col_num;
uint32_t vga::screen_row_num;

uint32_t default_rgb_color = RGB_COLOR_WHITE;

VGA_Modes vga_mode = TEXT;

VGA_Modes vga::get_vga_mode(void) {
    return vga_mode;
}

#pragma region Initialization

/// @brief Saves framebuffer info
/// @param fb_tag Framebuffer tag
void vga::init_framebuffer(const multiboot_tag_framebuffer* fb_tag) {  
    if (!fb_tag) {
        kprintf("No framebuffer tag!\n");
        kernel_panic("VGA error!");
        return;
    }  
    // Check if it's RGB mode
    if (fb_tag->framebuffer_type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
        kprintf("Invalid framebuffer tag (Framebuffer type: %u, expected: %u)!\n", fb_tag->framebuffer_type, MULTIBOOT_FRAMEBUFFER_TYPE_RGB);
        kprintf("Operating in VGA text mode!\n");
        return;
    }
    
    // Save framebuffer info
    framebuffer = (uint32_t*)fb_tag->framebuffer_addr;
    screen_width = fb_tag->framebuffer_width;
    screen_height = fb_tag->framebuffer_height;
    screen_pitch = fb_tag->framebuffer_pitch;
    screen_bpp = fb_tag->framebuffer_bpp;
    fb_size = screen_width * screen_height * (screen_bpp / 8);

    // Saving font info
    font_height = sizeof(font8x8_basic[0]) / sizeof(font8x8_basic[0][0]);
    font_width = sizeof(font8x8_basic[0][0]) * 8;
    // Saving screen size info
    screen_col_num = udiv64(screen_width, font_width);
    screen_row_num = udiv64(screen_height, font_height);

    vga_mode = FRAMEBUFFER;
}

void vga::init_backbuffer() {
    backbuffer = (uint32_t*)pmm::alloc_frame_by_size(vga::fb_size);
    if (backbuffer == nullptr) {
        kprintf(LOG_WARNING, "VGA: Could not allocate backbuffer. Scrolling will be slow.\n");
        return;
    }
    memset(backbuffer, 0, fb_size);
    kprintf(LOG_INFO, "VGA Backbuffer initialized");
}

#pragma endregion

#pragma region Bare metal print functions

/// @brief Displays a pixel to the screen
/// @param x X coordinate
/// @param y Y coordinate
/// @param color RGB(A) color value of pixel
void vga::put_pixel(const uint32_t x, const uint32_t y, const uint32_t color) {  // Note: uint32_t
    if (!framebuffer) return;
    if (x >= screen_width || y >= screen_height) return;

    // Calculate offsets
    uint64_t offset = y * screen_pitch + x * (screen_bpp / 8);
    uint8_t* pixel = (uint8_t*)framebuffer + offset;
    
    uint8_t* bb_pixel = nullptr;
    if (backbuffer) {
        bb_pixel = (uint8_t*)backbuffer + offset;
    }

    switch (screen_bpp) {
        // RGBA
        case 32:
            pixel[0] = (color >> 0) & 0xFF;   // Blue
            pixel[1] = (color >> 8) & 0xFF;   // Green  
            pixel[2] = (color >> 16) & 0xFF;  // Red
            pixel[3] = (color >> 24) & 0xFF;  // Alpha (or 0xFF for opaque)
            
            if (bb_pixel) {
                ((uint32_t*)bb_pixel)[0] = color;
            }
            break;
        // RGB
        case 24:
            pixel[0] = (color >> 0) & 0xFF;   // Blue
            pixel[1] = (color >> 8) & 0xFF;   // Green
            pixel[2] = (color >> 16) & 0xFF;  // Red
            
            if (bb_pixel) {
                bb_pixel[0] = pixel[0];
                bb_pixel[1] = pixel[1];
                bb_pixel[2] = pixel[2];
            }
            break;
        default:
            return;
    }
}

/// @brief Returns pixel color
uint32_t vga::get_pixel(const uint32_t x, const uint32_t y) {
    if (!framebuffer) return 0;
    if (x >= screen_width || y >= screen_height) return 0;

    uint8_t* base = (backbuffer) ? (uint8_t*)backbuffer : (uint8_t*)framebuffer;
    uint8_t* pixel_addr = base + y * screen_pitch + x * (screen_bpp / 8);

    if (screen_bpp == 32) {
        uint32_t color = *(uint32_t*)pixel_addr;
        // Mask out alpha
        return color & 0x00FFFFFF; 
    }
    
    return *(uint32_t*)pixel_addr;
}

#pragma endregion

