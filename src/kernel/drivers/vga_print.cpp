// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// vga_print.cpp
// In charge of printing to VGA buffer
// ========================================

#include <drivers/vga_print.hpp>
#include <drivers/vga.hpp>
#include <io.hpp>
#include <rtc.hpp>
#include <stdarg.h>
#include <lib/string_util.hpp>

// ====================
// Variables
// ====================
#pragma region Variables

// Notes if we initialized the title
bool initialized_title = false;

// Defining structure of a character
struct Char {
    uint8_t character;
    uint8_t color;
};

// Current address/location
size_t vga::col = 0;
size_t vga::row = 0;

uint8_t color = DEFAULT_FG_COLOR | (DEFAULT_BG_COLOR << 4); // Default colors

bool printingString = false;

#pragma endregion


// Print APIs

vga_coords printf(const char* fmt, ...) {
    VGAMode vga_mode = vga::get_vga_mode(); 
    if(vga_mode == VGA_TEXT) {
        va_list args;  // Declare a variable argument list
        va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

        vga_coords coords = vga::primitive_vprintf(fmt, args);

        va_end(args);  // Clean up the argument list
        return coords;
    }
    else if(vga_mode == FRAMEBUFFER) {
        va_list args;  // Declare a variable argument list
        va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

        vga_coords coords = vga::fb::kvprintf(STD_PRINT, default_rgb_color, fmt, args);

        va_end(args);  // Clean up the argument list
        return coords;
    }
}

vga_coords printf(const uint32_t color, const char* fmt, ...) {
    VGAMode vga_mode = vga::get_vga_mode(); 
    if(vga_mode == VGA_TEXT) {
        va_list args;  // Declare a variable argument list
        va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

        vga_coords coords = vga::primitive_printf(color, fmt, args);

        va_end(args);  // Clean up the argument list
        return coords;
    }
    else if(vga_mode == FRAMEBUFFER) {
        va_list args;  // Declare a variable argument list
        va_start(args, fmt);  // Initialize the argument list with the last fixed parameter

        vga_coords coords = vga::fb::kvprintf(STD_PRINT, color, fmt, args);

        va_end(args);  // Clean up the argument list
        return coords;
    }
}

vga_coords printf(const PrintTypes print_type, const char* fmt, ...) {
    VGAMode vga_mode = vga::get_vga_mode(); 
    vga_coords coords = {0, 0};

    if (vga_mode == VGA_TEXT) {
        va_list args;
        va_start(args, fmt);

        switch (print_type) {
            case STD_PRINT:
                coords = vga::primitive_vprintf(fmt, args);
                break;

            case LOG_INFO:
                vga::primitive_printf("[%s]: ", rtc::get_time());
                coords = vga::primitive_vprintf(fmt, args);
                break;

            case LOG_WARNING: {
                // Make a copy since primitive_warning will consume it
                coords = vga::primitive_vwarning(fmt, args);
                break;
            }

            case LOG_ERROR: {
                coords = vga::primitive_verror(fmt, args);
                break;
            }
        }

        va_end(args);
        return coords;
    }

    else if (vga_mode == FRAMEBUFFER) {
        va_list args;
        va_start(args, fmt);

        vga_coords coords = vga::fb::kvprintf(print_type, default_rgb_color, fmt, args);

        va_end(args);
        return coords;
    }
}


vga_coords printf(const PrintTypes print_type, const uint32_t color, const char* fmt, ...) {
    VGAMode vga_mode = vga::get_vga_mode(); 
    vga_coords coords = {0, 0};

    if (vga_mode == VGA_TEXT) {
        va_list args;
        va_start(args, fmt);

        switch (print_type) {
            case STD_PRINT:
                coords = vga::primitive_vprintf(fmt, args);
                break;

            case LOG_INFO:
                vga::primitive_printf("[%s]: ", rtc::get_time());
                coords = vga::primitive_vprintf(fmt, args);
                break;

            case LOG_WARNING: {
                // Make a copy since primitive_warning will consume it
                coords = vga::primitive_vwarning(fmt, args);
                break;
            }

            case LOG_ERROR: {
                coords = vga::primitive_verror(fmt, args);
                break;
            }
        }

        va_end(args);
        return coords;
    }

    else if (vga_mode == FRAMEBUFFER) {
        va_list args;
        va_start(args, fmt);

        vga_coords coords = vga::fb::kvprintf(print_type, color, fmt, args);

        va_end(args);
        return coords;
    }
}


// ====================
// Local functions
// ====================
#pragma region Local Functions

void update_cursor(const int row, const int col);

// Clears a specific size of symbols from a given coordinate
void vga::clear_region(const size_t _row, const size_t _col, const uint32_t len) {
    if(vga::get_vga_mode() == FRAMEBUFFER) {
        vga::fb::clear_text_region(_col, _row, len);
        return;
    }

    Char* vga = reinterpret_cast<Char*>(VGA_ADDRESS); // Getting VGA buffer

    // Clearing
    for(uint32_t i = 0; i < len; i++) {
        vga[_col + _row * NUM_COLS + i].character = ' ';
    }
}

// Inserts a string at a given coordinate
void vga::insert(size_t _row, size_t _col, const char* str, bool _update_cursor, uint8_t _color) {
    if(vga::get_vga_mode() == FRAMEBUFFER) {
        vga::fb::insert(_col, _row, _update_cursor, str);
        return;
    }

    Char* vga = reinterpret_cast<Char*>(VGA_ADDRESS); // VGA buffer

    // Saving coords
    size_t original_row = vga::row;
    size_t original_col = vga::col;
    
    // Set global cursor position
    vga::row = _row;
    vga::col = _col;

    // Setting to given color
    uint8_t original_color = color;
    color = _color;

    vga::primitive_printf(str);
    
    // Returning to old color
    color = original_color;

    if(!_update_cursor) {
        vga::row = original_row;
        vga::col = original_col;
    }

    // Update cursor to final position
    update_cursor(vga::row, vga::col);
}


// Clears indicated line
void _clear_row(const size_t row) {
    Char* buffer = reinterpret_cast<Char*>(VGA_ADDRESS);

    // Creating an empty struct
    Char empty {' ', color};

    // Iterating and clearing
    for(size_t col = 0; col < NUM_COLS; ++col) {
        buffer[col + NUM_COLS * row] = empty; // The newly created struct "empty"
    }
}


void print_newline(void) {
    Char* buffer = reinterpret_cast<Char*>(VGA_ADDRESS);

    vga::col = 0;

    // Only adding new line if possible
    if(vga::row < NUM_ROWS - 1) {
        ++vga::row;
    } else {
        // Scrolling the screen up and also keeping the title in screen
        for(size_t r = 1; r < NUM_ROWS; ++r) {
            for(size_t c = 0; c < NUM_COLS; ++c) {
                // Copying this row to the one above
                buffer[c + NUM_COLS * (r - 1)] = buffer[c + NUM_COLS * r];
            }   
        }

        _clear_row(NUM_ROWS - 1);
    }
    update_cursor(vga::row, vga::col);

}

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


void print_char(const char character) {
    Char* buffer = reinterpret_cast<Char*>(VGA_ADDRESS);

    // Handeling new line character input
    if(character == '\n') {
        print_newline();

        return;
    }

    if(vga::col >= NUM_COLS) {
        print_newline();
    }

    buffer[vga::col + NUM_COLS * vga::row] = {static_cast<uint8_t>(character), color};
    vga::col++; // Incrementing character number on this line

    /* If its printing a string we will update the cursor at the end of the string 
    // for performance reasons */
    if(!printingString)
        update_cursor(vga::row, vga::col); 
}

// Printing string to the screen
void print_str(const char* str) {
    printingString = true;

    // Calling the print_char function the same amount of times as the strings length
    for(size_t i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }

    update_cursor(vga::row, vga::col); 
    printingString = false;
}

#pragma endregion

// ====================
// Global functions
// ====================
namespace vga{

void init(void) {
    print_clear();
}

// Print formatted overloads

vga_coords primitive_vprintf(const char* format, va_list args) {
    vga_coords coords = {col, row}; // Saving coords

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
    return coords;
}

// Main printf function
vga_coords primitive_printf(const char* format, ...) {
    va_list args;  // Declare a variable argument list
    va_start(args, format);  // Initialize the argument list with the last fixed parameter

    primitive_vprintf(format, args);

    va_end(args);  // Clean up the argument list

    // Returning current coordinates
    return {col, row};
}

vga_coords primitive_printf(const uint8_t color, const char* format, ...) {
    // Saving color and swtching to given color
    uint8_t original_color = ::color;
    ::color = color;

    va_list args;  // Declare a variable argument list
    va_start(args, format);  // Initialize the argument list with the last fixed parameter

    primitive_vprintf(format, args);

    va_end(args);  // Clean up the argument list

    // Returning to original color
    ::color = original_color;

    // Returning current coordinates
    return {col, row};
}

vga_coords primitive_printf(const uint8_t color, const char* format, va_list args) {
    // Saving color and swtching to given color
    uint8_t original_color = ::color;
    ::color = color;

    primitive_vprintf(format, args);

    // Returning to original color
    ::color = original_color;

    // Returning current coordinates
    return {col, row};
}

// Main error function
vga_coords primitive_error(const char* format, ...) {
    // Saving current color
    uint8_t original_color = color;

    // Changing print color to make it more visible
    print_set_color(PRINT_COLOR_RED, PRINT_COLOR_BLACK);
    
    
    // Retreiving arguments
    va_list args;
    va_start(args, format);
    
    // Calling print
    primitive_vprintf(format, args);
    
    va_end(args);
    // Returning to original color
    color = original_color;

    // Returning current coordinates
    return {col, row};
}

// Main warning function
vga_coords primitive_warning(const char* format, ...) {
    // Saving current color
    uint8_t original_color = color;

    // Changing print color to make it more visible
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);

    // Retreiving arguments
    va_list args;
    va_start(args, format);

    // Calling print
    primitive_vprintf(format, args);

    va_end(args);

    // Returning to original color
    color = original_color;

    // Returning current coordinates
    return {col, row};
}

// Main error function
vga_coords primitive_verror(const char* format, va_list args) {
    // Saving current color
    uint8_t original_color = color;
    // Changing print color to make it more visible
    print_set_color(PRINT_COLOR_RED, PRINT_COLOR_BLACK);

    // Calling print
    primitive_vprintf(format, args);
    
    // Returning to original color
    color = original_color;
    // Returning current coordinates
    return {col, row};
}

// Main warning function
vga_coords primitive_vwarning(const char* format, va_list args) {
    // Saving current color
    uint8_t original_color = color;
    // Changing print color to make it more visible
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);

    // Calling print
    primitive_vprintf(format, args);

    // Returning to original color
    color = original_color;
    // Returning current coordinates
    return {col, row};
}

// Clears whole screen
void print_clear(void) {
    if(vga::get_vga_mode() == FRAMEBUFFER) {
        vga::fb::clear_screen();
        return;
    }

    // Iterating through all of the rows, and using clear_row
    for(size_t row = 0; row < NUM_ROWS; ++row) {
        _clear_row(row);
    }
}

// Backspace
void backspace(void) {
    if(vga::get_vga_mode() == FRAMEBUFFER) {
        vga::fb::backspace();
        return;
    }

    Char* buffer = reinterpret_cast<Char*>(VGA_ADDRESS);

    if (row * NUM_COLS + col > 0) {
        // Decrementing column variable
        col--;
        // If it gets bellow 0
        if(col <= 0) {
            row--;
            col = NUM_COLS;
        }

        // Replacing the standing character with a space and updating cursor
        buffer[col + NUM_COLS * row] = {static_cast<uint8_t>(' '), color};
        update_cursor(row, col);
    }
}

// Changing text color
void print_set_color(const uint8_t foreground, const uint8_t background) {
    color = foreground | (background << 4);
}

} // Namespace vga_print
