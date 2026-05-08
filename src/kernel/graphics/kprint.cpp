// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Kernel level print methods
// ========================================

#include <graphics/kprint.hpp>
#include <lib/conversions.hpp>
#include <io.hpp>

namespace vga
{
    struct Char {
        uint8_t character;
        uint8_t color;
    };

    uint8_t vgat_col = 0;
    uint8_t vgat_row = 0;
    bool printing_string = false;

    /// @brief Updates the crsor to specific coordinates
    static void update_cursor(const int row, const int col) {
        uint16_t pos = row * NUM_COLS + col;

        // Send the high byte of the cursor position
        cpu::outb(0x3D4, 14);               // Select high cursor byte
        cpu::outb(0x3D5, (pos >> 8) & 0xFF); // Send high byte

        // Send the low byte of the cursor position
        cpu::outb(0x3D4, 15);               // Select low cursor byte
        cpu::outb(0x3D5, pos & 0xFF);       // Send low byte
    }

    /// @brief Clears a row 
    /// @param row Row index
    static void clear_row(const uint8_t row) {
        Char* buffer = reinterpret_cast<Char*>(VGA_TEXT_ADDRESS);

        // Creating an empty struct
        Char empty {' ', VGA_TEXT_DEFAULT_COLOR};

        // Iterating and clearing
        for(uint8_t col = 0; col < NUM_COLS; ++col) {
            buffer[col + NUM_COLS * row] = empty; // The newly created struct "empty"
        }
    }

    /// @brief Prints a newline
    static void print_newline() {
        Char* buffer = reinterpret_cast<Char*>(VGA_TEXT_ADDRESS);

        vgat_col = 0;

        // Only adding new line if possible
        if(vgat_row < NUM_ROWS - 1) {
            ++vgat_row;
        } else {
            // Scrolling the screen up and also keeping the title in screen
            for(uint8_t r = 1; r < NUM_ROWS; ++r) {
                for(uint8_t c = 0; c < NUM_COLS; ++c) {
                    // Copying this row to the one above
                    buffer[c + NUM_COLS * (r - 1)] = buffer[c + NUM_COLS * r];
                }   
            }

            clear_row(NUM_ROWS - 1);
        }
        update_cursor(vgat_row, vgat_col);
    }

    /// @brief Prints a character to the screen
    static void kputch(const char ch) {
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

    /// @brief Prints a string to the screen
    void kputs(const char* str) {
        printing_string = true;

        // Calling the kputch function for every character
        for(uint8_t i = 0; str[i] != '\0'; i++) {
            kputch(str[i]);
        }

        update_cursor(vgat_row, vgat_col);
        printing_string = false;
    }

    void kvprintf(const char* fmt, va_list args) {
        while(*fmt) {
            if(*fmt == '%') {
                fmt++; // Moving to next char for the fmt specifier
                switch(*fmt) {
                    case 'd': {  // Signed 32-bit integer in decimal
                        int32_t num = va_arg(args, int32_t);
                        kputs(num_to_string(num));
                        break;
                    }
                    case 'u': {  // Unsigned 32-bit integer in decimal
                        uint32_t num = va_arg(args, uint32_t);
                        kputs(num_to_string(num));
                        break;
                    }
                    case 'l': {  // 64-bit integers (signed or unsigned)
                        fmt++;
                        if (*fmt == 'd') {  // Signed 64-bit integer in decimal
                            int64_t num = va_arg(args, int64_t);
                            kputs(num_to_string(num));
                        } else if (*fmt == 'u') {  // Unsigned 64-bit integer in decimal
                            uint64_t num = va_arg(args, uint64_t);
                            kputs(num_to_string(num));
                        } else if (*fmt == 'x') {  // Unsigned 64-bit integer in hexadecimal
                            uint64_t num = va_arg(args, uint64_t);
                            kputs("0x");
                            kputs(hex_to_string(num));
                        }
                        break;
                    }
                    case 'h': { // Unsigned 16-bit integer
                        uint32_t num = va_arg(args, uint32_t);
                        kputs("0x");
                        kputs(hex_to_string(num));
                        break;
                    }
                    case 'x': {  // Unsigned 32-bit integer in hexadecimal
                        uint32_t num = va_arg(args, uint32_t);
                        kputs("0x");
                        kputs(hex_to_string(num));
                        break;
                    }
                    case 'c': { // Character
                        char ch = (char)va_arg(args, int);  // Characters are promoted to int
                        kputch(ch);
                        break;
                    }
                    case 's': {  // String
                        const char* str = va_arg(args, const char*);
                        if (str) {
                            kputs(str);
                        } else {
                            kputs("(null)");
                        }
                        break;
                    }
                    default:  // Unknown fmt specifier
                        kputch('%');
                        kputch(*fmt);
                }
            }
            else kputch(*fmt);
            fmt++;
        }
    }

    void kclear() {
        for(uint8_t i = 0; i < NUM_ROWS; i++) clear_row(i);
    }
} // namespace vga

void kprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vga::kvprintf(fmt, args);

    va_end(args);
}

void kvprintf(const char* fmt, va_list args) {
    vga::kvprintf(fmt, args);
}