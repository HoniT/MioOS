// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef RENDER_HPP
#define RENDER_HPP

#include <stdint.h>

#define TASKBAR_HEIGHT 30

namespace gui {
    // Shapes
    void draw_rect(const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h, const uint32_t color);
    void draw_rect_outline(const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h, const uint32_t color);
    void draw_circle(const uint32_t cx, const uint32_t cy, const uint32_t radius, const uint32_t color);
    void draw_circle_outline(const uint32_t cx, const uint32_t cy, const uint32_t radius, const uint32_t color);
    void draw_line(const uint32_t x0, const uint32_t y0, const uint32_t x1, const uint32_t y1, const uint32_t color);
    void draw_triangle(const uint32_t x0, const uint32_t y0,
                        const uint32_t x1, const uint32_t y1,
                        const uint32_t x2, const uint32_t y2,
                        const uint32_t color);
    void draw_char(const uint32_t x, const uint32_t y, const char c, const uint32_t color);
} // namespace gui

#endif // RENDER_HPP
