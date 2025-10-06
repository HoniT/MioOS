// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// vga.cpp
// Sets up a driver for VGA framebuffer drawing and printf
// ========================================

#include <drivers/vga.hpp>
#include <drivers/vga_print.hpp>
#include <rtc.hpp>
#include <interrupts/kernel_panic.hpp>
#include <lib/string_util.hpp>
#include <lib/math.hpp>

uint32_t* vga::framebuffer = nullptr;
uint32_t vga::fb_size;
static uint32_t screen_width;
static uint32_t screen_height;
static uint32_t screen_pitch;
static uint8_t screen_bpp; // Bits per pixel

static uint8_t font_height;
static uint8_t font_width;
static uint32_t screen_col_num;
static uint32_t screen_row_num;

uint32_t default_rgb_color = RGB_COLOR_WHITE;

#pragma region Initialization

/// @brief Saves framebuffer info
/// @param fb_tag Framebuffer tag
void vga::init_framebuffer(const multiboot_tag_framebuffer* fb_tag) {  
    if (!fb_tag) {
        printf("No framebuffer tag!\n");
        return;
    }  
    // Check if it's RGB mode
    if (fb_tag->framebuffer_type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
        printf("Invalid framebuffer tag!\n");
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
}

/// @brief Gets current VGA mode
VGAMode vga::get_vga_mode(void) {
    return framebuffer ? FRAMEBUFFER : VGA_TEXT;
}

#pragma endregion

#pragma region Primitive print functions

/// @brief Displays a pixel to the screen
/// @param x X coordinate
/// @param y Y coordinate
/// @param color RGB(A) color value of pixel
void vga::fb::put_pixel(const uint32_t x, const uint32_t y, const uint32_t color) {  // Note: uint32_t
    if (!framebuffer) return;
    if (x >= screen_width || y >= screen_height) return;

    uint8_t* pixel = (uint8_t*)framebuffer + y * screen_pitch + x * (screen_bpp / 8);

    switch (screen_bpp) {
        // RGBA
        case 32:
            pixel[0] = (color >> 0) & 0xFF;   // Blue
            pixel[1] = (color >> 8) & 0xFF;   // Green  
            pixel[2] = (color >> 16) & 0xFF;  // Red
            pixel[3] = (color >> 24) & 0xFF;  // Alpha (or 0xFF for opaque)
            break;
        // RGB
        case 24:
            pixel[0] = (color >> 0) & 0xFF;   // Blue
            pixel[1] = (color >> 8) & 0xFF;   // Green
            pixel[2] = (color >> 16) & 0xFF;  // Red
            break;
        default:
            // kernel_panic("Unrecognized screen BPP!");
            return;
    }
}

/// @brief Displays an ASCII character on screen
/// @param x X coordinate
/// @param y Y coordinate
/// @param c ASCII character
/// @param color RGB(A) color value of character
void vga::fb::draw_char(const uint32_t x, const uint32_t y, const char c, const uint32_t color) {
    if (c < 0 || c > 127) return; // ASCII bounds

    int font_height = sizeof(font8x8_basic[0]) / sizeof(font8x8_basic[0][0]);
    int font_width = sizeof(font8x8_basic[0][0]) * 8;

    for (int row = 0; row < font_height; row++) {
        auto bits = font8x8_basic[(uint8_t)c - 32][row];

        for (int col = 0; col < font_width; col++) {
            if (bits & (1 << (font_width - 1 - col))) {
                vga::fb::put_pixel(x + col, y + row, color);
            }
        }
    }
}

/// @brief Prints text on screen
/// @param x X coordinate
/// @param y Y coordinate
/// @param str String to display
/// @param color RGB(A) color value of text
void vga::fb::draw_text(uint32_t x, uint32_t y, const char* str, const uint32_t color) {
    while (*str) {
        // New line
        if(x + font_width >= screen_width) {
            y += font_height;
            x = 0;
        }

        vga::fb::draw_char(x, y, *str, color);
        x += font_width; // Move right for next char
        str++;
    }
}

#pragma endregion

// Current row and column 
int vga::fb::col_num = 0;
int vga::fb::row_num = 0;

#pragma region Additional screen functions

/// @brief Clears a given row
/// @param row Row index
void clear_row(const size_t row) {
    if(row >= screen_row_num) return;

    // Iterating and clearing
    for (size_t y = row * font_height; y < (row + 1) * font_height; ++y) {
        for (size_t x = 0; x < screen_width; ++x) {
            vga::framebuffer[y * screen_width + x] = 0;
        }
    }
}


/// @brief Goes to a new line
void newline(void) {
    vga::fb::col_num = 0;
    
    // Only adding new line if possible
    if(vga::fb::row_num < screen_row_num - 1) {
        vga::fb::row_num++;
    } else {
        // Scroll up by font_height pixels
        for (size_t y = font_height; y < screen_height; ++y) {
            for (size_t x = 0; x < screen_width; ++x) {
                vga::framebuffer[(y - font_height) * screen_width + x] = vga::framebuffer[y * screen_width + x];
            }
        }

        // Clearing up last row for the new lines text
        clear_row(screen_row_num - 1);
    }
}

/// @brief Clears a given region of text
/// @param col Start column of region to clear
/// @param row Start row of region to clear
/// @param len Lengthof region to clear (number of characters)
void vga::fb::clear_text_region(size_t col, size_t row, size_t len) {
    if (row >= screen_row_num || col >= screen_col_num) return;

    size_t chars_remaining = len;
    size_t current_row = row;
    size_t current_col = col;

    while (chars_remaining > 0 && current_row < screen_row_num) {
        size_t chars_on_row = screen_col_num - current_col;
        if (chars_on_row > chars_remaining)
            chars_on_row = chars_remaining;

        size_t x_start = current_col * font_width;
        size_t x_end   = x_start + chars_on_row * font_width;
        size_t y_start = current_row * font_height;
        size_t y_end   = y_start + font_height;

        if (x_end > screen_width)  x_end = screen_width;
        if (y_end > screen_height) y_end = screen_height;

        for (size_t y = y_start; y < y_end; ++y) {
            for (size_t x = x_start; x < x_end; ++x) {
                framebuffer[y * screen_width + x] = RGB_COLOR_BLACK;
            }
        }

        chars_remaining -= chars_on_row;
        current_row++;
        current_col = 0;
    }
}

/// @brief Deletes last character
void vga::fb::backspace(void) {
    if(col_num - 1 < 0) {
        col_num = screen_col_num - 1;
        row_num--;
    } else col_num--;

    vga::fb::clear_text_region(vga::fb::col_num, vga::fb::row_num, 1);
}

/// @brief Clears whole screen
void vga::fb::clear_screen(void) {
    for(uint32_t i = 0; i < vga::fb_size / 4; i++) {
        vga::framebuffer[i] = 0;
    }
    col_num = row_num = 0;
}

/// @brief Inserts text at a given column and row
/// @param col Column
/// @param row Row
/// @param color RGB(A) color value
/// @param fmt Format
void vinsert(const size_t col, const size_t row, const uint32_t color, const bool update_pos, const char* fmt, va_list args) {
    if(col >= screen_col_num || row >= screen_row_num) return;

    vga::fb::vkprintf_at(col, row, STD_PRINT, color, update_pos, fmt, args);
}

/// @brief Inserts text at a given column and row
/// @param col Column
/// @param row Row
/// @param color RGB(A) color value
/// @param fmt Format
void vga::fb::insert(const size_t col, const size_t row, const uint32_t color, const bool update_pos, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vinsert(col, row, color, update_pos, fmt, args);
    va_end(args);
}

/// @brief Inserts text at a given column and row
/// @param col Column
/// @param row Row
/// @param fmt Format
void vga::fb::insert(const size_t col, const size_t row, const bool update_pos, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vinsert(col, row, default_rgb_color, update_pos, fmt, args);
    va_end(args);
}

#pragma endregion

#pragma region Basic output

/// @brief Prints a string to the screen
/// @param color RGB(A) color value
/// @param char Character to print
void vga::fb::kputchar(const uint32_t color, const char c) {
    if(c == '\n') {
        newline();
        return;
    }
    
    vga::fb::draw_char(col_num * font_width, row_num * font_height, c, color);
    col_num++;
    // New line
    if(col_num * font_width + font_width >= screen_width) {
        newline();
    }
}

/// @brief Prints a string to the screen
/// @param color RGB(A) color value
/// @param str String to print
void vga::fb::kputs(const uint32_t color, const char* str) {
    while (*str) {
        vga::fb::kputchar(color, *str);
        str++;
    }
}

/// @brief Prints formatted text
/// @param fmt Format
/// @param Parameters
vga_coords vga::fb::kprintf(const char* fmt, ...) {
    vga_coords coords = {col_num, row_num}; // Saving coords

    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    // Calling base function
    vga::fb::kvprintf(STD_PRINT, default_rgb_color, fmt, args);

    va_end(args);  // Clean up the argument list
    return coords;
}

/// @brief Prints formatted text
/// @param color RGB(A) color value
/// @param fmt Format
/// @param Parameters
vga_coords vga::fb::kprintf(const uint32_t color, const char* fmt, ...) {
    vga_coords coords = {col_num, row_num}; // Saving coords

    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    // Calling base function
    vga::fb::kvprintf(STD_PRINT, color, fmt, args);

    va_end(args);  // Clean up the argument list
    return coords;
}

/// @brief Prints formatted text
/// @param print_type Print type (enum PrintTypes)
/// @param fmt Format
/// @param Parameters
vga_coords vga::fb::kprintf(const PrintTypes print_type, const char* fmt, ...) {
    vga_coords coords = {col_num, row_num}; // Saving coords

    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    // Calling base function
    vga::fb::kvprintf(print_type, default_rgb_color, fmt, args);

    va_end(args);  // Clean up the argument list
    return coords;
}

/// @brief Prints formatted text
/// @param color RGB(A) color value
/// @param print_type Print type (enum PrintTypes)
/// @param fmt Format
/// @param Parameters
vga_coords vga::fb::kprintf(const PrintTypes print_type, const uint32_t color, const char* fmt, ...) {
    vga_coords coords = {col_num, row_num}; // Saving coords

    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    // Calling base function
    vga::fb::kvprintf(print_type, color, fmt, args);

    va_end(args);  // Clean up the argument list
    return coords;
}

/// @brief Prints at a given column and row
/// @param col Column to print at
/// @param row Row to print at
/// @param print_type Print type
/// @param color RGB(A) color value
/// @param update_pos If true we set the current position at the end of the inserted text
/// @param fmt Format
/// @return Coordinates printed at (same as parameter col and row)
vga_coords vga::fb::kprintf_at(const size_t col, const size_t row, const PrintTypes print_type, const uint32_t color, const bool update_pos, const char* fmt, ...) {
    if(col >= screen_col_num || row >= screen_row_num) return {0, 0};
    vga_coords original_coords = {col_num, row_num}; // Saving original coords

    // Changing to given coords
    col_num = col;
    row_num = row;

    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    // Calling base function
    vga::fb::kvprintf(print_type, color, fmt, args);

    // Returning to old coords
    col_num = original_coords.col;
    row_num = original_coords.row;

    va_end(args);  // Clean up the argument list
    return {col, row};
}

/// @brief Prints at a given column and row
/// @param col Column to print at
/// @param row Row to print at
/// @param print_type Print type
/// @param color RGB(A) color value
/// @param update_pos If true we set the current position at the end of the inserted text
/// @param fmt Format
/// @param args Variadic list of args
/// @return Coordinates printed at
vga_coords vga::fb::vkprintf_at(const size_t col, const size_t row, const PrintTypes print_type, const uint32_t color, const bool update_pos, const char* fmt, va_list args) {
    if(col >= screen_col_num || row >= screen_row_num) return {0, 0};
    vga_coords original_coords = {col_num, row_num}; // Saving original coords

    // Changing to given coords
    col_num = col;
    row_num = row;

    // Calling base function
    vga::fb::kvprintf(print_type, color, fmt, args);

    // Returning to old coords
    if(!update_pos) {
        col_num = original_coords.col;
        row_num = original_coords.row;
    }

    return {col_num, row_num};
}

/// @brief Base function for formatted printing
/// @param color RGB(A) color value
/// @param print_type Print type (enum PrintTypes)
/// @param fmt Format
/// @param args Variadic list of arguments
vga_coords vga::fb::kvprintf(const PrintTypes print_type, const uint32_t color, const char* fmt, const va_list args) {
    vga_coords coords = {col, row}; // Saving coords

    // Any special additional text depending on the text type
    switch(print_type) {
        case STD_PRINT:
            break;

        case LOG_INFO:
            vga::fb::kprintf("[%s]: ", rtc::get_time());
            break;

        case LOG_WARNING:
            vga::fb::kprintf("[%s]: ", rtc::get_time());
            vga::fb::kputs(RGB_COLOR_YELLOW, "Warning: ");
            break;

        case LOG_ERROR:
            vga::fb::kprintf("[%s]: ", rtc::get_time());
            vga::fb::kputs(RGB_COLOR_RED, "Error: ");
            break;

        default:
            // Invalid type
            return {0, 0};
    }

    while(*fmt) {
        // Format specifier
        if(*fmt == '%') {
            fmt++;
            switch(*fmt) {
                case '%': {
                    vga::fb::kputchar(color, '%');
                    break;
                }
                // Shorter numbers (char - short) 
                case 'h': {
                    fmt++;
                    if(*fmt == 'h') {
                        fmt++;
                        switch(*fmt) {
                            // Int8_t
                            case 'd': {
                                int num = va_arg(args, int);
                                vga::fb::kputs(color, num_to_string((int8_t)num));
                                break;
                            }
                            // Uint8_t
                            case 'u': {
                                unsigned int num = va_arg(args, unsigned int);
                                vga::fb::kputs(color, num_to_string((uint8_t)num));
                                break;
                            }
                            // Uint8_t (hex)
                            case 'x': {
                                unsigned int num = va_arg(args, unsigned int);
                                vga::fb::kputs(color, "0x");
                                vga::fb::kputs(color, hex_to_string((uint8_t)num));
                                break;
                            }
                        }
                    } else {
                        switch(*fmt) {
                            // Int16_t
                            case 'd': {
                                int num = va_arg(args, int);
                                vga::fb::kputs(color, num_to_string((int16_t)num));
                                break;
                            }
                            // Uint16_t
                            case 'u': {
                                unsigned int num = va_arg(args, unsigned int);
                                vga::fb::kputs(color, num_to_string((uint16_t)num));
                                break;
                            }
                            // Uint16_t (hex)
                            case 'x': {
                                unsigned int num = va_arg(args, unsigned int);
                                vga::fb::kputs(color, "0x");
                                vga::fb::kputs(color, hex_to_string((uint16_t)num));
                                break;
                            }
                        }
                    }
                    break;
                }
                // Large numbers (long - long long)
                case 'l': {
                    fmt++;
                    if(*fmt == 'l') {
                        fmt++;
                        switch(*fmt) {
                            // Int64_t
                            case 'd': {
                                long long num = va_arg(args, long long);
                                vga::fb::kputs(color, num_to_string((int64_t)num));
                                break;
                            }
                            // Uint64_t
                            case 'u': {
                                unsigned long long num = va_arg(args, unsigned long long);
                                vga::fb::kputs(color, num_to_string((uint64_t)num));
                                break;
                            }
                            // Uint64_t (hex)
                            case 'x': {
                                unsigned long long num = va_arg(args, unsigned long long);
                                vga::fb::kputs(color, "0x");
                                vga::fb::kputs(color, hex_to_string((uint64_t)num));
                                break;
                            }
                        }
                    } else {
                        switch(*fmt) {
                            // Int32_t
                            case 'd': {
                                long num = va_arg(args, long);
                                vga::fb::kputs(color, num_to_string((int32_t)num));
                                break;
                            }
                            // Uint32_t
                            case 'u': {
                                unsigned long num = va_arg(args, unsigned long);
                                vga::fb::kputs(color, num_to_string((uint32_t)num));
                                break;
                            }
                            // Uint32_t (hex)
                            case 'x': {
                                unsigned long num = va_arg(args, unsigned long);
                                vga::fb::kputs(color, "0x");
                                vga::fb::kputs(color, hex_to_string((uint32_t)num));
                                break;
                            }
                        }
                    }
                    break;
                }
                // Int32_t
                case 'd': {
                    int32_t num = va_arg(args, int);
                    vga::fb::kputs(color, num_to_string(num));
                    break;
                }
                // Uint32_t
                case 'u': {
                    uint32_t num = va_arg(args, unsigned int);
                    vga::fb::kputs(color, num_to_string(num));
                    break;
                }
                // Uint32_t (hex)
                case 'x': {
                    uint32_t num = va_arg(args, unsigned int);
                    vga::fb::kputs(color, "0x");
                    vga::fb::kputs(color, hex_to_string(num));
                    break;
                }
                // Character
                case 'c': {
                    char c = (char)va_arg(args, int);
                    vga::fb::kputchar(color, c);
                    break;
                }
                // String
                case 's': {
                    const char* str = va_arg(args, const char*);
                    vga::fb::kputs(color, str);
                    break;
                }
                case 'S': {
                    data::string* str = va_arg(args, data::string*);
                    vga::fb::kputs(color, str->c_str());
                    break;
                }
                // Pointer
                case 'p': {
                    void* p = va_arg(args, void*);
                    vga::fb::kputs(color, "0x");
                    vga::fb::kputs(color, hex_to_string((uint32_t)p));
                    break;
                }
            }
        }
        else vga::fb::kputchar(color, *fmt);
        fmt++;
    }
    return coords;
}


#pragma endregion
