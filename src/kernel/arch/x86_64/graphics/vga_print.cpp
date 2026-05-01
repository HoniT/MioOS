// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <arch/x86_64/graphics/vga_print.hpp>
#include <lib/conversions.hpp>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

struct Char {
    uint8_t character;
    uint8_t color;
};

size_t vgat_col = 0;
size_t vgat_row = 0;
bool printing_string = false;

// Updates the cursor to fit the active coordinates/address 
void update_cursor(const int row, const int col) {
    uint16_t pos = row * NUM_COLS + col;

    // Send the high byte of the cursor position
    // io::outPortB(0x3D4, 14);               // Select high cursor byte
    // io::outPortB(0x3D5, (pos >> 8) & 0xFF); // Send high byte

    // // Send the low byte of the cursor position
    // io::outPortB(0x3D4, 15);               // Select low cursor byte
    // io::outPortB(0x3D5, pos & 0xFF);       // Send low byte
}

// Clears indicated line
void vgat_clear_row(const size_t row) {
    Char* buffer = reinterpret_cast<Char*>(VGA_TEXT_ADDRESS);

    // Creating an empty struct
    Char empty {' ', VGA_TEXT_DEFAULT_COLOR};

    // Iterating and clearing
    for(size_t col = 0; col < NUM_COLS; ++col) {
        buffer[col + NUM_COLS * row] = empty; // The newly created struct "empty"
    }
}

void print_newline(void) {
    Char* buffer = reinterpret_cast<Char*>(VGA_TEXT_ADDRESS);

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

static void putchar(const char ch) {
    Char* buffer = reinterpret_cast<Char*>(VGA_TEXT_ADDRESS);

    // Handeling new line character input
    if(ch == '\n') {
        print_newline();

        return;
    }

    if(vgat_col >= NUM_COLS) {
        print_newline();
    }

    buffer[vgat_col + NUM_COLS * vgat_row] = {static_cast<uint8_t>(ch), VGA_TEXT_DEFAULT_COLOR};
    vgat_col++; // Incrementing character number on this line

    /* If its printing a string we will update the cursor at the end of the string 
    // for performance reasons */
    if(!printing_string)
        update_cursor(vgat_row, vgat_col); 
}

// Printing string to the screen
void putstr(const char* str) {
    printing_string = true;

    // Calling the putchar function for every character
    for(size_t i = 0; str[i] != '\0'; i++) {
        putchar(str[i]);
    }

    update_cursor(vgat_row, vgat_col);
    printing_string = false;
}

static void vprintf(const char* fmt, va_list args) {
    while(*fmt) {
        if(*fmt == '%') {
            fmt++; // Moving to next char for the fmt specifier
            switch(*fmt) {
                case 'd': {  // Signed 32-bit integer in decimal
                    int32_t num = va_arg(args, int32_t);
                    putstr(num_to_string(num));
                    break;
                }
                case 'u': {  // Unsigned 32-bit integer in decimal
                    uint32_t num = va_arg(args, uint32_t);
                    putstr(num_to_string(num));
                    break;
                }
                case 'l': {  // 64-bit integers (signed or unsigned)
                    fmt++;
                    if (*fmt == 'd') {  // Signed 64-bit integer in decimal
                        int64_t num = va_arg(args, int64_t);
                        putstr(num_to_string(num));
                    } else if (*fmt == 'u') {  // Unsigned 64-bit integer in decimal
                        uint64_t num = va_arg(args, uint64_t);
                        putstr(num_to_string(num));
                    } else if (*fmt == 'x') {  // Unsigned 64-bit integer in hexadecimal
                        uint64_t num = va_arg(args, uint64_t);
                        putstr("0x");
                        putstr(hex_to_string(num));
                    }
                    break;
                }
                case 'h': { // Unsigned 16-bit integer
                    uint32_t num = va_arg(args, uint32_t);
                    putstr("0x");
                    putstr(hex_to_string(num));
                    break;
                }
                case 'x': {  // Unsigned 32-bit integer in hexadecimal
                    uint32_t num = va_arg(args, uint32_t);
                    putstr("0x");
                    putstr(hex_to_string(num));
                    break;
                }
                case 'c': { // Character
                    char ch = (char)va_arg(args, int);  // Characters are promoted to int
                    putchar(ch);
                    break;
                }
                case 's': {  // String
                    const char* str = va_arg(args, const char*);
                    if (str) {
                        putstr(str);
                    } else {
                        putstr("(null)");
                    }
                    break;
                }
                default:  // Unknown fmt specifier
                    putchar('%');
                    putchar(*fmt);
            }
        }
        else putchar(*fmt);
        fmt++;
    }
}

/// @brief A simple fomratted print utility for VGA text mode
///        
void x86_64::vga::printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vprintf(fmt, args);

    va_end(args);
}
