// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// vga_print.cpp
// In charge of printing to VGA buffer
// ========================================

#include <drivers/vga_print.hpp>
#include <lib/io.hpp>
#include <stdarg.h>

// ====================
// Variables
// ====================
#pragma region Variables

// Size constraints
const size_t NUM_COLS = 80;
const size_t NUM_ROWS = 25;

// Defining structure of a character
struct Char {
    uint8_t character;
    uint8_t color;
};

// Current address/location
size_t col = 0;
size_t row = 0;

uint8_t color = PRINT_COLOR_WHITE | (PRINT_COLOR_BLACK << 4); // Standard white on black

bool printingString = false;

#pragma endregion


// ====================
// Local functions
// ====================
#pragma region Local Functions

// ====================
// Helper functions
// ====================
#pragma region Helper Functions

// Clears indicated line
void clear_row(const size_t row) {
    Char* buffer = reinterpret_cast<Char*>(VGA_ADDRESS);

    // Creating an empty struct
    Char empty {' ', color};

    // Iterating and clearing
    for(size_t col = 0; col < NUM_COLS; ++col) {
        buffer[col + NUM_COLS * row] = empty; // The newly created struct "empty"
    }
}

void update_cursor(const int row, const int col);

void print_newline(void) {
    Char* buffer = reinterpret_cast<Char*>(VGA_ADDRESS);

    col = 0;

    // Only adding new line if possible
    if(row < NUM_ROWS - 1) {
        ++row;
    } else {
        // Scrolling the screen up

        for(size_t r = 1; r < NUM_ROWS; ++r) {
            for(size_t c = 0; c < NUM_COLS; ++c) {
                // Copying this row to the one above
                buffer[c + NUM_COLS * (r - 1)] = buffer[c + NUM_COLS * r];
            }   
        }

        clear_row(NUM_ROWS - 1);
    }
    update_cursor(row, col);

}

// Convert a single nibble (4 bits) to its hex character representation
char nibble_to_hex(const uint8_t nibble) {
    if (nibble < 10) {
        return '0' + nibble;
    } else {
        return 'A' + (nibble - 10);
    }
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

#pragma endregion

// ====================
// Local Print functions
// ====================
#pragma region Local Print Functions

void print_char(const char character) {
    Char* buffer = reinterpret_cast<Char*>(VGA_ADDRESS);
    
    // Handeling new line character input
    if(character == '\n') {
        print_newline();

        return;
    }

    if(col >= NUM_COLS) {
        print_newline();
    }

    buffer[col + NUM_COLS * row] = {static_cast<uint8_t>(character), color};
    col++; // Incrementing character number on this line

    /* If its printing a string we will update the cursor at the end of the string 
    // for performance reasons */
    if(!printingString)
        update_cursor(row, col); 
}

// Printing string to the screen
void print_str(const char* str) {
    printingString = true;

    // Calling the print_char function the same amount of times as the strings length
    for(size_t i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }

    update_cursor(row, col); 
    printingString = false;
}

void print_hex(const int32_t num) {
    print_str("0x"); // Prefix for hex numbers

    // Print each nibble (4 bits) as a hex digit
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (num >> (i * 4)) & 0xF;  // Extract the current nibble
        print_char(nibble_to_hex(nibble));        // Print the corresponding hex digit
    }
}

void print_hex(const int64_t num) {
    print_str("0x"); // Prefix for hex numbers

    // Print each nibble (4 bits) as a hex digit
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = (num >> (i * 4)) & 0xF;  // Extract the current nibble
        print_char(nibble_to_hex(nibble));        // Print the corresponding hex digit
    }
}

#pragma endregion

#pragma endregion

// ====================
// Global functions
// ====================
namespace vga{

void init(void) {
    print_set_color(PRINT_COLOR_GREEN, PRINT_COLOR_BLACK);
    print_clear();
    
    printf(" ==================================== IoOS ==================================== \n");
}

// Print formatted overloads

// Print a number in decimal format
void print_decimal(uint32_t num) {
    char buffer[11]; // Maximum digits for a 32-bit uint32_t (10 digits + null terminator)
    int i = 10;

    buffer[i--] = '\0'; // Null-terminate the string

    // Handle 0 explicitly
    if (num == 0) {
        buffer[i] = '0';
        print_char(buffer[i]);
        return;
    }

    // Convert number to string
    while (num > 0 && i >= 0) {
        buffer[i--] = '0' + (num % 10);
        num /= 10;
    }

    // Print the number
    for (int j = i + 1; buffer[j] != '\0'; j++) {
        print_char(buffer[j]);
    }
}

// Print a number in decimal format
void print_decimal(uint64_t num) {
    char buffer[21]; // Maximum digits for a 64-bit uint64_t (20 digits + null terminator)
    int i = 20;

    buffer[i--] = '\0'; // Null-terminate the string

    // Handle 0 explicitly
    if (num == 0) {
        buffer[i] = '0';
        print_char(buffer[i]);
        return;
    }

    // Convert number to string
    while (num > 0 && i >= 0) {
        buffer[i--] = '0' + (num % 10);
        num /= 10;
    }

    // Print the number
    for (int j = i + 1; buffer[j] != '\0'; j++) {
        print_char(buffer[j]);
    }
}

// Print a number in decimal format
void print_decimal(int32_t num) {
    char buffer[11]; // Maximum digits for a 32-bit uint32_t (10 digits + null terminator)
    int i = 10;

    buffer[i--] = '\0'; // Null-terminate the string

    // Handle 0 explicitly
    if (num == 0) {
        buffer[i] = '0';
        print_char(buffer[i]);
        return;
    }

    // Convert number to string
    while (num > 0 && i >= 0) {
        buffer[i--] = '0' + (num % 10);
        num /= 10;
    }

    // Print the number
    for (int j = i + 1; buffer[j] != '\0'; j++) {
        print_char(buffer[j]);
    }
}

// Prints a number in decimal format
void print_decimal(int64_t num) {
    char buffer[21]; // Maximum digits for a 64-bit uint64_t (20 digits + null terminator)
    int i = 20;

    buffer[i--] = '\0'; // Null-terminate the string

    // Handle 0 explicitly
    if (num == 0) {
        buffer[i] = '0';
        print_char(buffer[i]);
        return;
    }

    // Convert number to string
    while (num > 0 && i >= 0) {
        buffer[i--] = '0' + (num % 10);
        num /= 10;
    }

    // Print the number
    for (int j = i + 1; buffer[j] != '\0'; j++) {
        print_char(buffer[j]);
    }
}

// Main printf function
void printf(const char* format, ...) {
    va_list args;  // Declare a variable argument list
    va_start(args, format);  // Initialize the argument list with the last fixed parameter

    while (*format) {
        if (*format == '%') {
            format++;  // Move to the format specifier
            switch (*format) {
                case 'd': {  // Signed 32-bit integer in decimal
                    int32_t num = va_arg(args, int32_t);
                    print_decimal(num);
                    break;
                }
                case 'u': {  // Unsigned 32-bit integer in decimal
                    uint32_t num = va_arg(args, uint32_t);
                    print_decimal(num);
                    break;
                }
                case 'l': {  // 64-bit integers (signed or unsigned)
                    format++;
                    if (*format == 'd') {  // Signed 64-bit integer in decimal
                        int64_t num = va_arg(args, int64_t);
                        print_decimal(num);
                    } else if (*format == 'u') {  // Unsigned 64-bit integer in decimal
                        uint64_t num = va_arg(args, uint64_t);
                        print_decimal(num);
                    } else if (*format == 'x') {  // Unsigned 64-bit integer in hexadecimal
                        uint64_t num = va_arg(args, uint64_t);
                        print_hex(int64_t(num));
                    }
                    break;
                }
                case 'x': {  // Unsigned 32-bit integer in hexadecimal
                    uint32_t num = va_arg(args, uint32_t);
                    print_hex(int32_t(num));
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
                default:  // Unknown format specifier
                    print_char('%');
                    print_char(*format);
            }
        } else {
            print_char(*format);  // Print normal characters
        }
        format++;
    }

    va_end(args);  // Clean up the argument list
}

// Main error function
void error(const char* format, ...) {
    // Saving current color
    uint8_t original_color = color;

    // Changing print color to make it more visible
    print_set_color(PRINT_COLOR_RED, PRINT_COLOR_BLACK);

    // Printing text
    va_list args;  // Declare a variable argument list
    va_start(args, format);  // Initialize the argument list with the last fixed parameter

    while (*format) {
        if (*format == '%') {
            format++;  // Move to the format specifier
            switch (*format) {
                case 'd': {  // Signed 32-bit integer in decimal
                    int32_t num = va_arg(args, int32_t);
                    print_decimal(num);
                    break;
                }
                case 'u': {  // Unsigned 32-bit integer in decimal
                    uint32_t num = va_arg(args, uint32_t);
                    print_decimal(num);
                    break;
                }
                case 'l': {  // 64-bit integers (signed or unsigned)
                    format++;
                    if (*format == 'd') {  // Signed 64-bit integer in decimal
                        int64_t num = va_arg(args, int64_t);
                        print_decimal(num);
                    } else if (*format == 'u') {  // Unsigned 64-bit integer in decimal
                        uint64_t num = va_arg(args, uint64_t);
                        print_decimal(num);
                    } else if (*format == 'x') {  // Unsigned 64-bit integer in hexadecimal
                        uint64_t num = va_arg(args, uint64_t);
                        print_hex(int64_t(num));
                    }
                    break;
                }
                case 'x': {  // Unsigned 32-bit integer in hexadecimal
                    uint32_t num = va_arg(args, uint32_t);
                    print_hex(int32_t(num));
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
                default:  // Unknown format specifier
                    print_char('%');
                    print_char(*format);
            }
        } else {
            print_char(*format);  // Print normal characters
        }
        format++;
    }

    va_end(args);  // Clean up the argument list

    // Returning to original color
    color = original_color;
}

// Clears whole screen
void print_clear(void) {
    // Iterating through all of the rows, and using clear_row
    for(size_t row = 0; row < NUM_ROWS; ++row) {
        clear_row(row);
    }
}

// Backspace
void backspace(void) {
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
