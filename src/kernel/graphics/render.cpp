// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// render.cpp
// Renderer for graphics
// ========================================

#include <graphics/render.hpp>
#include <graphics/vga_print.hpp>
#include <drivers/vga.hpp>
#include <lib/math.hpp> 
#include <lib/data/list.hpp>

void gui::put_pixel_alpha(const uint32_t x, const uint32_t y, const uint32_t color, const uint8_t alpha) {
    if (alpha == 0) return;
    if (alpha == 255) {
        vga::put_pixel(x, y, color);
        return;
    }

    if (!vga::framebuffer) return;
    if (x >= vga::screen_width || y >= vga::screen_height) return;

    uint8_t* pixel = (uint8_t*)vga::framebuffer + y * vga::screen_pitch + x * (vga::screen_bpp / 8);

    uint32_t fg_b = (color >> 0) & 0xFF;
    uint32_t fg_g = (color >> 8) & 0xFF;
    uint32_t fg_r = (color >> 16) & 0xFF;
    
    // We use 256 for the shift optimization, so we need inv_alpha to sum to 256
    uint32_t inv_alpha = 256 - alpha; 

    switch (vga::screen_bpp) {
        // RGBA
        case 32: {
            uint32_t bg_b = pixel[0];
            uint32_t bg_g = pixel[1];
            uint32_t bg_r = pixel[2];

            pixel[0] = (uint8_t)((fg_b * alpha + bg_b * inv_alpha) >> 8); // Blue
            pixel[1] = (uint8_t)((fg_g * alpha + bg_g * inv_alpha) >> 8); // Green
            pixel[2] = (uint8_t)((fg_r * alpha + bg_r * inv_alpha) >> 8); // Red
            break;
        }
        // RGB
        case 24: {
            uint32_t bg_b = pixel[0];
            uint32_t bg_g = pixel[1];
            uint32_t bg_r = pixel[2];

            pixel[0] = (uint8_t)((fg_b * alpha + bg_b * inv_alpha) >> 8); // Blue
            pixel[1] = (uint8_t)((fg_g * alpha + bg_g * inv_alpha) >> 8); // Green
            pixel[2] = (uint8_t)((fg_r * alpha + bg_r * inv_alpha) >> 8); // Red
            break;
        }
        default:
            return;
    }
}

void gui::draw_rect(const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h, const uint32_t color, const uint8_t alpha) {
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++)
            gui::put_pixel_alpha(xx, yy, color, alpha);
}

/// @brief Draws a rectangle behind pixels of front color
/// @param x X coordinate
/// @param y Y coordinate
/// @param w Width
/// @param h Height
/// @param color Rectangle color
/// @param back_color Will only change pixels with this color
/// @param alpha Rectangle alpha
void gui::draw_rect_behind(const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h, const uint32_t color, const uint32_t back_color, const uint8_t alpha) {
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++)
            if(vga::get_pixel(xx, yy) == back_color)
                gui::put_pixel_alpha(xx, yy, color, alpha);
}

void gui::draw_rect_outline(const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h, const uint32_t color, const uint8_t alpha) {
    for (uint32_t xx = x; xx < x + w; xx++) {
        gui::put_pixel_alpha(xx, y, color, alpha);
        gui::put_pixel_alpha(xx, y + h - 1, color, alpha);
    }
    for (uint32_t yy = y; yy < y + h; yy++) {
        gui::put_pixel_alpha(x, yy, color, alpha);
        gui::put_pixel_alpha(x + w - 1, yy, color, alpha);
    }
}

void gui::draw_circle(const uint32_t cx, const uint32_t cy, const uint32_t radius, const uint32_t color, const uint8_t alpha) {
    for (int y = -static_cast<int>(radius); y <= static_cast<int>(radius); y++) {
        for (int x = -static_cast<int>(radius); x <= static_cast<int>(radius); x++) {
            if (x * x + y * y <= static_cast<int>(radius) * static_cast<int>(radius))
                gui::put_pixel_alpha(cx + x, cy + y, color, alpha);
        }
    }
}

void gui::draw_circle_outline(const uint32_t cx, const uint32_t cy, const uint32_t radius, const uint32_t color, const uint8_t alpha) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        gui::put_pixel_alpha(cx + x, cy + y, color, alpha);
        gui::put_pixel_alpha(cx + y, cy + x, color, alpha);
        gui::put_pixel_alpha(cx - y, cy + x, color, alpha);
        gui::put_pixel_alpha(cx - x, cy + y, color, alpha);
        gui::put_pixel_alpha(cx - x, cy - y, color, alpha);
        gui::put_pixel_alpha(cx - y, cy - x, color, alpha);
        gui::put_pixel_alpha(cx + y, cy - x, color, alpha);
        gui::put_pixel_alpha(cx + x, cy - y, color, alpha);

        y += 1;
        if (err <= 0)
            err += 2 * y + 1;
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void gui::draw_line(const uint32_t x0, const uint32_t y0, const uint32_t x1, const uint32_t y1, const uint32_t color, const uint8_t alpha) {
    int dx = abs((int)x1 - (int)x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs((int)y1 - (int)y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    int x = x0;
    int y = y0;

    while (true) {
        gui::put_pixel_alpha(x, y, color, alpha);
        if (x == (int)x1 && y == (int)y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

void gui::draw_triangle(const uint32_t x0, const uint32_t y0,
                        const uint32_t x1, const uint32_t y1,
                        const uint32_t x2, const uint32_t y2,
                        const uint32_t color, const uint8_t alpha) {
                            
    int minX = min((int[]){x0, x1, x2});
    int maxX = max((int[]){x0, x1, x2});
    int minY = min((int[]){y0, y1, y2});
    int maxY = max((int[]){y0, y1, y2});

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            int w0 = (x1 - x0) * (y - y0) - (y1 - y0) * (x - x0);
            int w1 = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);
            int w2 = (x0 - x2) * (y - y2) - (y0 - y2) * (x - x2);
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0))
                gui::put_pixel_alpha(x, y, color, alpha);
        }
    }
}

/// @brief Displays an ASCII character on screen
/// @param x X coordinate
/// @param y Y coordinate
/// @param c ASCII character
/// @param color RGB(A) color value of character
void gui::draw_char(const uint32_t x, const uint32_t y, const char c, const uint32_t color, const uint8_t alpha) {
    if (c < 0 || c > 127) return; // ASCII bounds

    int font_height = 8;
    int font_width = 8;

    for (int row = 0; row < font_height; row++) {
        auto bits = font8x8_basic[(uint8_t)c - 32][row];

        for (int col = 0; col < font_width; col++) {
            if (bits & (1 << (font_width - 1 - col))) {
                gui::put_pixel_alpha(x + col, y + row, color, alpha);
            }
        }
    }
}