// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// vga_print.cpp
// In charge of printing to VGA buffer
// ========================================

#include <graphics/vga_print.hpp>
#include <graphics/render.hpp>
#include <drivers/vga.hpp>
#include <x86/io.hpp>
#include <drivers/rtc.hpp>
#include <stdarg.h>
#include <lib/string_util.hpp>

// Current row and column 
int vga::col_num = 0;
int vga::row_num = 0;

// Current VGA text section (null if whole screen)
static vga_section* curr_section = nullptr;

#pragma region Additional screen functions

/// @brief Creates a VGA text section
/// @param startX Start X coordinate (NOT COLUMN)
/// @param startY Start Y coordinate (NOT ROW)
/// @param endX End X coordinate (NOT COLUMN)
/// @param endY End Y coordinate (NOT ROW)
vga_section vga::create_section(uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY) {
    return vga_section(startX, startY, endX, endY);
}

/// @brief Creates a VGA text section
/// @param start Start VGA coords (COL, ROW)
/// @param end End VGA coords (COL, ROW)
vga_section vga::create_section(vga_coords start, vga_coords end) {
    return vga_section(start.col * font_width, start.row * font_height, (end.col + 1) * font_width, (end.row + 1) * font_height);
}

/// @brief Clears a given row
/// @param row Row index
void clear_row(const size_t row) {
    if (curr_section) {
        size_t max_rows = (curr_section->endY - curr_section->startY) / vga::font_height;
        if (row >= max_rows) return;

        size_t y_start = curr_section->startY + (row * vga::font_height);
        size_t y_end = y_start + vga::font_height;

        // Clip to section bounds
        if (y_end > curr_section->endY) y_end = curr_section->endY;

        for (size_t y = y_start; y < y_end; ++y) {
            for (size_t x = curr_section->startX; x < curr_section->endX; ++x) {
                vga::framebuffer[y * vga::screen_width + x] = 0;
            }
        }
        return;
    }

    if(row >= vga::screen_row_num) return;

    // Iterating and clearing
    for (size_t y = row * vga::font_height; y < (row + 1) * vga::font_height; ++y) {
        for (size_t x = 0; x < vga::screen_width; ++x) {
            vga::framebuffer[y * vga::screen_width + x] = 0;
        }
    }
}

/// @brief Goes to a new line
void newline(void) {
    if (curr_section) {
        curr_section->col = 0;
        
        size_t max_rows = (curr_section->endY - curr_section->startY) / vga::font_height;

        // Only adding new line if possible
        if (curr_section->row < max_rows - 1) {
            curr_section->row++;
        } else {
            // Scroll up within section
            for (size_t y = curr_section->startY + vga::font_height; y < curr_section->endY; ++y) {
                for (size_t x = curr_section->startX; x < curr_section->endX; ++x) {
                    vga::framebuffer[(y - vga::font_height) * vga::screen_width + x] = vga::framebuffer[y * vga::screen_width + x];
                }
            }
            // Clearing up last row of the section
            clear_row(max_rows - 1);
        }
        return;
    }

    vga::col_num = 0;
    
    // Only adding new line if possible
    if(vga::row_num < vga::screen_row_num - 1) {
        vga::row_num++;
    } else {
        // Scroll up by vga::font_height pixels
        for (size_t y = vga::font_height; y < vga::screen_height; ++y) {
            for (size_t x = 0; x < vga::screen_width; ++x) {
                vga::framebuffer[(y - vga::font_height) * vga::screen_width + x] = vga::framebuffer[y * vga::screen_width + x];
            }
        }

        // Clearing up last row for the new lines text
        clear_row(vga::screen_row_num - 1);
    }
}

/// @brief Clears a given region of text
/// @param col Start column of region to clear
/// @param row Start row of region to clear
/// @param len Lengthof region to clear (number of characters)
void vga::clear_text_region(const size_t col, const size_t row, const size_t len) {
    if (curr_section) {
        size_t max_cols = (curr_section->endX - curr_section->startX) / vga::font_width;
        size_t max_rows = (curr_section->endY - curr_section->startY) / vga::font_height;

        if (row >= max_rows || col >= max_cols) return;

        size_t chars_remaining = len;
        size_t current_row = row;
        size_t current_col = col;

        while (chars_remaining > 0 && current_row < max_rows) {
            size_t chars_on_row = max_cols - current_col;
            if (chars_on_row > chars_remaining)
                chars_on_row = chars_remaining;

            size_t x_start = curr_section->startX + (current_col * vga::font_width);
            size_t x_end   = x_start + (chars_on_row * vga::font_width);
            size_t y_start = curr_section->startY + (current_row * vga::font_height);
            size_t y_end   = y_start + vga::font_height;

            if (x_end > curr_section->endX) x_end = curr_section->endX;
            if (y_end > curr_section->endY) y_end = curr_section->endY;

            for (size_t y = y_start; y < y_end; ++y) {
                for (size_t x = x_start; x < x_end; ++x) {
                    vga::framebuffer[y * vga::screen_width + x] = RGB_COLOR_BLACK;
                }
            }

            chars_remaining -= chars_on_row;
            current_row++;
            current_col = 0;
        }
        return;
    }

    if (row >= vga::screen_row_num || col >= vga::screen_col_num) return;

    size_t chars_remaining = len;
    size_t current_row = row;
    size_t current_col = col;

    while (chars_remaining > 0 && current_row < vga::screen_row_num) {
        size_t chars_on_row = vga::screen_col_num - current_col;
        if (chars_on_row > chars_remaining)
            chars_on_row = chars_remaining;

        size_t x_start = current_col * vga::font_width;
        size_t x_end   = x_start + chars_on_row * vga::font_width;
        size_t y_start = current_row * vga::font_height;
        size_t y_end   = y_start + vga::font_height;

        if (x_end > vga::screen_width)  x_end = vga::screen_width;
        if (y_end > vga::screen_height) y_end = vga::screen_height;

        for (size_t y = y_start; y < y_end; ++y) {
            for (size_t x = x_start; x < x_end; ++x) {
                vga::framebuffer[y * vga::screen_width + x] = RGB_COLOR_BLACK;
            }
        }

        chars_remaining -= chars_on_row;
        current_row++;
        current_col = 0;
    }
}

/// @brief Deletes last character
void vga::backspace(void) {
    if (curr_section) {
        size_t max_cols = (curr_section->endX - curr_section->startX) / vga::font_width;
        
        if (curr_section->col == 0) {
            if (curr_section->row > 0) {
                curr_section->col = max_cols - 1;
                curr_section->row--;
            }
        } else {
            curr_section->col--;
        }
        clear_text_region(curr_section->col, curr_section->row, 1);
        return;
    }

    if(vga::col_num - 1 < 0) {
        vga::col_num = vga::screen_col_num - 1;
        vga::row_num--;
    } else vga::col_num--;

    clear_text_region(vga::col_num, vga::row_num, 1);
}

/// @brief Clears whole screen
void vga::clear_screen(void) {
    for(uint32_t i = 0; i < vga::fb_size / 4; i++) {
        vga::framebuffer[i] = 0;
    }
    vga::col_num = vga::row_num = 0;
}

/// @brief Inserts text at a given column and row
/// @param col Column
/// @param row Row
/// @param color RGB(A) color value
/// @param fmt Format
void vga::insert(const size_t col, const size_t row, const uint32_t color, const bool update_pos, const char* fmt, ...) {
    if(col >= vga::screen_col_num || row >= vga::screen_row_num) return;

    va_list args;
    va_start(args, fmt);

    kvprintf_at(col, row, STD_PRINT, color, update_pos, fmt, args);
    va_end(args);
}

/// @brief Inserts text at a given column and row
/// @param col Column
/// @param row Row
/// @param fmt Format
void vga::insert(const size_t col, const size_t row, const bool update_pos, const char* fmt, ...) {
    if(col >= vga::screen_col_num || row >= vga::screen_row_num) return;

    va_list args;
    va_start(args, fmt);

    kvprintf_at(col, row, STD_PRINT, default_rgb_color, update_pos, fmt, args);
    va_end(args);
}

#pragma endregion

#pragma region Basic output

/// @brief Prints a string to the screen
/// @param color RGB(A) color value
/// @param char Character to print
void kputchar(const uint32_t color, const char c) {
    if(c == '\n') {
        newline();
        return;
    }
    
    if (curr_section) {
        size_t draw_x = curr_section->startX + (curr_section->col * vga::font_width);
        size_t draw_y = curr_section->startY + (curr_section->row * vga::font_height);

        // Safety check to ensure we don't draw outside section bounds
        if (draw_x + vga::font_width <= curr_section->endX && draw_y + vga::font_height <= curr_section->endY) {
            gui::draw_char(draw_x, draw_y, c, color);
        }

        curr_section->col++;

        // New line check for section
        size_t current_width_px = (curr_section->col * vga::font_width) + vga::font_width;
        size_t section_width_px = curr_section->endX - curr_section->startX;

        if(current_width_px >= section_width_px) {
            newline();
        }
        return;
    }

    gui::draw_char(vga::col_num * vga::font_width, vga::row_num * vga::font_height, c, color);
    vga::col_num++;
    // New line
    if(vga::col_num * vga::font_width + vga::font_width >= vga::screen_width) {
        newline();
    }
}

/// @brief Prints a string to the screen
/// @param color RGB(A) color value
/// @param str String to print
void kputs(const uint32_t color, const char* str) {
    while (*str) {
        kputchar(color, *str);
        str++;
    }
}

/// @brief Prints formatted text
/// @param fmt Format
vga_coords kprintf(const char* fmt, ...) {
    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    // Calling base function
    kvprintf(nullptr, STD_PRINT, default_rgb_color, fmt, args);

    va_end(args);  // Clean up the argument list
    
    if (curr_section) return {curr_section->col, curr_section->row};
    return {vga::col_num, vga::row_num};
}

/// @brief Prints formatted text
/// @param color RGB(A) color value
/// @param fmt Format
vga_coords kprintf(const uint32_t color, const char* fmt, ...) {
    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    // Calling base function
    kvprintf(nullptr, STD_PRINT, color, fmt, args);

    va_end(args);  // Clean up the argument list
    
    if (curr_section) return {curr_section->col, curr_section->row};
    return {vga::col_num, vga::row_num};
}

/// @brief Prints formatted text
/// @param print_type Print type (enum PrintTypes)
/// @param fmt Format
vga_coords kprintf(const PrintTypes print_type, const char* fmt, ...) {
    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    // Calling base function
    kvprintf(nullptr, print_type, default_rgb_color, fmt, args);

    va_end(args);  // Clean up the argument list
    
    if (curr_section) return {curr_section->col, curr_section->row};
    return {vga::col_num, vga::row_num};
}

/// @brief Prints formatted text
/// @param color RGB(A) color value
/// @param print_type Print type (enum PrintTypes)
/// @param fmt Format
vga_coords kprintf(const PrintTypes print_type, const uint32_t color, const char* fmt, ...) {
    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    // Calling base function
    kvprintf(nullptr, print_type, color, fmt, args);

    va_end(args);  // Clean up the argument list
    
    if (curr_section) return {curr_section->col, curr_section->row};
    return {vga::col_num, vga::row_num};
}

/// @brief Prints formatted text
/// @param sect VGA text section
/// @param fmt Format
vga_coords kprintf(vga_section& sect, const char* fmt, ...) {
    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    kvprintf(&sect, STD_PRINT, default_rgb_color, fmt, args);

    va_end(args);  // Clean up the argument list
    return {sect.col, sect.row};
}

/// @brief Prints formatted text
/// @param sect VGA text section
/// @param print_type Print type (enum PrintTypes)
/// @param fmt Format
vga_coords kprintf(vga_section& sect, const PrintTypes print_type, const char* fmt, ...) {
    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    kvprintf(&sect, print_type, default_rgb_color, fmt, args);

    va_end(args);  // Clean up the argument list
    return {sect.col, sect.row};
}

/// @brief Prints formatted text
/// @param sect VGA text section
/// @param color RGB(A) color value
/// @param fmt Format
vga_coords kprintf(vga_section& sect, const uint32_t color, const char* fmt, ...) {
    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    kvprintf(&sect, STD_PRINT, color, fmt, args);

    va_end(args);  // Clean up the argument list
    return {sect.col, sect.row};
}

/// @brief Prints formatted text
/// @param sect VGA text section
/// @param print_type Print type (enum PrintTypes)
/// @param color RGB(A) color value
/// @param fmt Format
vga_coords kprintf(vga_section& sect, const PrintTypes print_type, const uint32_t color, const char* fmt, ...) {
    va_list args;  // Declare a variable argument list
    va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

    kvprintf(&sect, print_type, color, fmt, args);

    va_end(args);  // Clean up the argument list
    return {sect.col, sect.row};
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
vga_coords kvprintf_at(const size_t col, const size_t row, const PrintTypes print_type, const uint32_t color, const bool update_pos, const char* fmt, va_list args) {
    if(col >= vga::screen_col_num || row >= vga::screen_row_num) return {0, 0};
    vga_coords original_coords = {vga::col_num, vga::row_num}; // Saving original coords

    // Changing to given coords
    vga::col_num = col;
    vga::row_num = row;

    // Calling base function
    kvprintf(nullptr, print_type, color, fmt, args);

    // Returning to old coords
    if(!update_pos) {
        vga::col_num = original_coords.col;
        vga::row_num = original_coords.row;
    }

    return {vga::col_num, vga::row_num};
}

/// @brief Base function for formatted printing
/// @param color RGB(A) color value
/// @param print_type Print type (enum PrintTypes)
/// @param sect VGA section
/// @param fmt Format
/// @param args Variadic list of arguments
void kvprintf(vga_section* sect, const PrintTypes print_type, uint32_t color, const char* fmt, const va_list args) {
    if(vga::get_vga_mode() == TEXT) {
        vga::vgat_vprintf(fmt, args);
        return;
    }

    // Set the global section pointer for the duration of this print
    vga_section* prev_section = curr_section;
    if (sect != nullptr) {
        curr_section = sect;
    }
    
    // Any special additional text depending on the text type
    switch(print_type) {
        case STD_PRINT:
            break;

        case LOG_INFO:
            kprintf("[%s]: ", rtc::get_time());
            break;

        case LOG_WARNING:
            kprintf("[%s]: ", rtc::get_time());
            kputs(RGB_COLOR_YELLOW, "Warning: ");
            break;

        case LOG_ERROR:
            kprintf("[%s]: ", rtc::get_time());
            kputs(RGB_COLOR_RED, "Error: ");
            break;

        default:
            // Invalid type
            // Restore section before returning
            curr_section = prev_section;
            return;
    }

    while(*fmt) {
        // Format specifier
        if(*fmt == '%') {
            fmt++;
            switch(*fmt) {
                case '%': {
                    kputchar(color, '%');
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
                                kputs(color, num_to_string((int8_t)num));
                                break;
                            }
                            // Uint8_t
                            case 'u': {
                                unsigned int num = va_arg(args, unsigned int);
                                kputs(color, num_to_string((uint8_t)num));
                                break;
                            }
                            // Uint8_t (hex)
                            case 'x': {
                                unsigned int num = va_arg(args, unsigned int);
                                kputs(color, "0x");
                                kputs(color, hex_to_string((uint8_t)num));
                                break;
                            }
                        }
                    } else {
                        switch(*fmt) {
                            // Int16_t
                            case 'd': {
                                int num = va_arg(args, int);
                                kputs(color, num_to_string((int16_t)num));
                                break;
                            }
                            // Uint16_t
                            case 'u': {
                                unsigned int num = va_arg(args, unsigned int);
                                kputs(color, num_to_string((uint16_t)num));
                                break;
                            }
                            // Uint16_t (hex)
                            case 'x': {
                                unsigned int num = va_arg(args, unsigned int);
                                kputs(color, "0x");
                                kputs(color, hex_to_string((uint16_t)num));
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
                                kputs(color, num_to_string((int64_t)num));
                                break;
                            }
                            // Uint64_t
                            case 'u': {
                                unsigned long long num = va_arg(args, unsigned long long);
                                kputs(color, num_to_string((uint64_t)num));
                                break;
                            }
                            // Uint64_t (hex)
                            case 'x': {
                                unsigned long long num = va_arg(args, unsigned long long);
                                kputs(color, "0x");
                                kputs(color, hex_to_string((uint64_t)num));
                                break;
                            }
                        }
                    } else {
                        switch(*fmt) {
                            // Int32_t
                            case 'd': {
                                long num = va_arg(args, long);
                                kputs(color, num_to_string((int32_t)num));
                                break;
                            }
                            // Uint32_t
                            case 'u': {
                                unsigned long num = va_arg(args, unsigned long);
                                kputs(color, num_to_string((uint32_t)num));
                                break;
                            }
                            // Uint32_t (hex)
                            case 'x': {
                                unsigned long num = va_arg(args, unsigned long);
                                kputs(color, "0x");
                                kputs(color, hex_to_string((uint32_t)num));
                                break;
                            }
                        }
                    }
                    break;
                }
                // Int32_t
                case 'd': {
                    int32_t num = va_arg(args, int);
                    kputs(color, num_to_string(num));
                    break;
                }
                // Uint32_t
                case 'u': {
                    uint32_t num = va_arg(args, unsigned int);
                    kputs(color, num_to_string(num));
                    break;
                }
                // Uint32_t (hex)
                case 'x': {
                    uint32_t num = va_arg(args, unsigned int);
                    kputs(color, "0x");
                    kputs(color, hex_to_string(num));
                    break;
                }
                // Character
                case 'c': {
                    char c = (char)va_arg(args, int);
                    kputchar(color, c);
                    break;
                }
                // String
                case 's': {
                    const char* str = va_arg(args, const char*);
                    kputs(color, str);
                    break;
                }
                case 'S': {
                    data::string* str = va_arg(args, data::string*);
                    kputs(color, str->c_str());
                    break;
                }
                case 'C': { // Color change
                    color = va_arg(args, unsigned int);
                    break;
                }
                // Pointer
                case 'p': {
                    void* p = va_arg(args, void*);
                    kputs(color, "0x");
                    kputs(color, hex_to_string((uint32_t)p));
                    break;
                }
            }
        }
        else kputchar(color, *fmt);
        fmt++;
    }

    // Restore previous section (if any)
    curr_section = prev_section;
}

#pragma endregion

// VGA text mode is not in use anymore
#pragma region VGA Text Mode

#ifdef IO_HPP

// Updates the cursor to fit the active coordinates/address 
void update_cursor(const int row, const int col) {
    uint16_t pos = row * NUM_COLS + col;

    // Send the high byte of the cursor position
    io::outPortB(0x3D4, 14);               // Select high cursor byte
    io::outPortB(0x3D5, (pos >> 8) & 0xFF); // Send high byte

    // Send the low byte of the cursor position
    io::outPortB(0x3D4, 15);               // Select low cursor byte
    io::outPortB(0x3D5, pos & 0xFF);       // Send low byte
}

#endif // IO_HPP

struct Char {
    uint8_t character;
    uint8_t color;
};

size_t vgat_col = 0;
size_t vgat_row = 0;

bool printingString = false;

// Clears indicated line
void vgat_clear_row(const size_t row) {
    Char* buffer = reinterpret_cast<Char*>(VGA_ADDRESS);

    // Creating an empty struct
    Char empty {' ', VGAT_COLOR};

    // Iterating and clearing
    for(size_t col = 0; col < NUM_COLS; ++col) {
        buffer[col + NUM_COLS * row] = empty; // The newly created struct "empty"
    }
}


void print_newline(void) {
    Char* buffer = reinterpret_cast<Char*>(VGA_ADDRESS);

    vgat_col = 0;

    // Only adding new line if possible
    if(vgat_row < NUM_ROWS - 1) {
        ++vgat_row;
    } else {
        // Scrolling the screen up and also keeping the title in screen
        for(size_t r = 1; r < NUM_ROWS; ++r) {
            for(size_t c = 0; c < NUM_COLS; ++c) {
                // Copying this row to the one above
                buffer[c + NUM_COLS * (r - 1)] = buffer[c + NUM_COLS * r];
            }   
        }

        vgat_clear_row(NUM_ROWS - 1);
    }
    update_cursor(vgat_row, vgat_col);

}

void print_char(const char ch) {
    Char* buffer = reinterpret_cast<Char*>(VGA_ADDRESS);

    // Handeling new line character input
    if(ch == '\n') {
        print_newline();

        return;
    }

    if(vgat_col >= NUM_COLS) {
        print_newline();
    }

    buffer[vgat_col + NUM_COLS * vgat_row] = {static_cast<uint8_t>(ch), VGAT_COLOR};
    vgat_col++; // Incrementing character number on this line

    /* If its printing a string we will update the cursor at the end of the string 
    // for performance reasons */
    if(!printingString)
        update_cursor(vgat_row, vgat_col); 
}

// Printing string to the screen
void print_str(const char* str) {
    printingString = true;

    // Calling the print_char function the same amount of times as the strings length
    for(size_t i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }

    update_cursor(vgat_row, vgat_col);
    printingString = false;
}

/// @brief VGA text mode printf
/// @param format Format
/// @param args Variadic list of arguments
void vga::vgat_vprintf(const char* format, va_list args) {
    while (*format) {
        if (*format == '%') {
            format++;  // Move to the format specifier
            switch (*format) {
                case 'd': {  // Signed 32-bit integer in decimal
                    int32_t num = va_arg(args, int32_t);
                    print_str(num_to_string(num));
                    break;
                }
                case 'u': {  // Unsigned 32-bit integer in decimal
                    uint32_t num = va_arg(args, uint32_t);
                    print_str(num_to_string(num));
                    break;
                }
                case 'l': {  // 64-bit integers (signed or unsigned)
                    format++;
                    if (*format == 'd') {  // Signed 64-bit integer in decimal
                        int64_t num = va_arg(args, int64_t);
                        print_str(num_to_string(num));
                    } else if (*format == 'u') {  // Unsigned 64-bit integer in decimal
                        uint64_t num = va_arg(args, uint64_t);
                        print_str(num_to_string(num));
                    } else if (*format == 'x') {  // Unsigned 64-bit integer in hexadecimal
                        uint64_t num = va_arg(args, uint64_t);
                        print_str("0x");
                        print_str(hex_to_string(num));
                    }
                    break;
                }
                case 'h': { // Unsigned 16-bit integer
                    uint32_t num = va_arg(args, uint32_t);
                    print_str("0x");
                    print_str(hex_to_string(num));
                    break;
                }
                case 'x': {  // Unsigned 32-bit integer in hexadecimal
                    uint32_t num = va_arg(args, uint32_t);
                    print_str("0x");
                    print_str(hex_to_string(num));
                    break;
                }
                case 'c': {  // Character
                    char ch = (char)va_arg(args, int);  // Characters are promoted to int
                    print_char(ch);
                    break;
                }
                case 's': {  // String
                    const char* str = va_arg(args, const char*);
                    if (str) {
                        print_str(str);
                    } else {
                        print_str("(null)");
                    }
                    break;
                }
                case 'S': {  // Custom string
                    data::string* str = va_arg(args, data::string*);
                    if (str) {
                        print_str(str->c_str());
                    } else {
                        print_str("(null)");
                    }
                    break;
                }
                default:  // Unknown format specifier
                    print_char('%');
                    print_char(*format);
            }
        } else {
            print_char(*format);  // Print normal characters
        }
        format++;
    }
}

#pragma endregion
