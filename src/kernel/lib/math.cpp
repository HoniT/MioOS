// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// math.cpp
// In charge of math functions
// ========================================

#include <lib/math.hpp>

// Turns a char* hex number to a uint32_t
uint32_t hex_to_uint32(const char* hexStr) {
    uint32_t result = 0;

    // Skip the "0x" or "0X" prefix if present
    if (hexStr[0] == '0' && (hexStr[1] == 'x' || hexStr[1] == 'X')) {
        hexStr += 2;
    }

    while (*hexStr) {
        result <<= 4; // Shift the result left by 4 bits to make space for the next digit
        if (*hexStr >= '0' && *hexStr <= '9') {
            result |= (*hexStr - '0');
        } else if (*hexStr >= 'A' && *hexStr <= 'F') {
            result |= (*hexStr - 'A' + 10);
        } else if (*hexStr >= 'a' && *hexStr <= 'f') {
            result |= (*hexStr - 'a' + 10);
        } else {
            // Invalid character encountered, handle the error as needed
            return 0;
        }
        hexStr++;
    }

    return result;
}


// Perform 64-bit unsigned division
uint64_t udiv64(uint64_t dividend, uint64_t divisor) {
    if (divisor == 0) {
        // Handle division by zero, return 0 or raise an error
        return 0;
    }

    uint64_t quotient = 0;
    uint64_t remainder = 0;

    for (int i = 63; i >= 0; --i) {
        remainder = (remainder << 1) | ((dividend >> i) & 1);
        if (remainder >= divisor) {
            remainder -= divisor;
            quotient |= (1ULL << i);
        }
    }

    return quotient;
}

// Perform 64-bit unsigned modulus
uint64_t umod64(uint64_t dividend, uint64_t divisor) {
    if (divisor == 0) {
        // Handle division by zero, return 0 or raise an error
        return 0;
    }

    uint64_t remainder = 0;

    for (int i = 63; i >= 0; --i) {
        remainder = (remainder << 1) | ((dividend >> i) & 1);
        if (remainder >= divisor) {
            remainder -= divisor;
        }
    }

    return remainder;
}
