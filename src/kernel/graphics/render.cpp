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

void gui::draw_rect(const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h, const uint32_t color) {
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++)
            vga::put_pixel(xx, yy, color);
}

void gui::draw_rect_outline(const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h, const uint32_t color) {
    for (uint32_t xx = x; xx < x + w; xx++) {
        vga::put_pixel(xx, y, color);
        vga::put_pixel(xx, y + h - 1, color);
    }
    for (uint32_t yy = y; yy < y + h; yy++) {
        vga::put_pixel(x, yy, color);
        vga::put_pixel(x + w - 1, yy, color);
    }
}

void gui::draw_circle(const uint32_t cx, const uint32_t cy, const uint32_t radius, const uint32_t color) {
    for (int y = -static_cast<int>(radius); y <= static_cast<int>(radius); y++) {
        for (int x = -static_cast<int>(radius); x <= static_cast<int>(radius); x++) {
            if (x * x + y * y <= static_cast<int>(radius) * static_cast<int>(radius))
                vga::put_pixel(cx + x, cy + y, color);
        }
    }
}

void gui::draw_circle_outline(const uint32_t cx, const uint32_t cy, const uint32_t radius, const uint32_t color) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        vga::put_pixel(cx + x, cy + y, color);
        vga::put_pixel(cx + y, cy + x, color);
        vga::put_pixel(cx - y, cy + x, color);
        vga::put_pixel(cx - x, cy + y, color);
        vga::put_pixel(cx - x, cy - y, color);
        vga::put_pixel(cx - y, cy - x, color);
        vga::put_pixel(cx + y, cy - x, color);
        vga::put_pixel(cx + x, cy - y, color);

        y += 1;
        if (err <= 0)
            err += 2 * y + 1;
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void gui::draw_line(const uint32_t x0, const uint32_t y0, const uint32_t x1, const uint32_t y1, const uint32_t color) {
    int dx = abs((int)x1 - (int)x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs((int)y1 - (int)y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    int x = x0;
    int y = y0;

    while (true) {
        vga::put_pixel(x, y, color);
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
                        const uint32_t color) {
                            
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
                vga::put_pixel(x, y, color);
        }
    }
}

/// @brief Displays an ASCII character on screen
/// @param x X coordinate
/// @param y Y coordinate
/// @param c ASCII character
/// @param color RGB(A) color value of character
void gui::draw_char(const uint32_t x, const uint32_t y, const char c, const uint32_t color) {
    if (c < 0 || c > 127) return; // ASCII bounds

    int font_height = 8;
    int font_width = 8;

    for (int row = 0; row < font_height; row++) {
        auto bits = font8x8_basic[(uint8_t)c - 32][row];

        for (int col = 0; col < font_width; col++) {
            if (bits & (1 << (font_width - 1 - col))) {
                vga::put_pixel(x + col, y + row, color);
            }
        }
    }
}
