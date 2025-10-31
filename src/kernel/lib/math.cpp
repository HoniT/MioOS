// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// math.cpp
// In charge of math functions
// ========================================

#include <lib/math.hpp>

uint32_t abs(int x) {
    return x >= 0 ? x : -x;
}

int min(int a, int b) {
    return a <= b ? a : b;
}

int max(int a, int b) {
    return a >= b ? a : b;
}

int min(int* nums) {
    int count = sizeof(nums) / sizeof(int);
    int min = INT32_MAX;
    for(int i = 0; i < count; i++) if(nums[i] < min) min = nums[i];
    return min;
}

int max(int* nums) {
    int count = sizeof(nums) / sizeof(int);
    int max = INT32_MIN;
    for(int i = 0; i < count; i++) if(nums[i] > max) max = nums[i];
    return max;
}

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

// Turns a char* hex number to a uint64_t
uint64_t hex_to_uint64(const char* hexStr) {
    uint64_t result = 0;

    // Skip the "0x" or "0X" prefix if present
    if (hexStr[0] == '0' && (hexStr[1] == 'x' || hexStr[1] == 'X')) {
        hexStr += 2;
    }

    while (*hexStr) {
        result <<= 4; // Shift left by 4 bits to make space for the next digit
        if (*hexStr >= '0' && *hexStr <= '9') {
            result |= (*hexStr - '0');
        } else if (*hexStr >= 'A' && *hexStr <= 'F') {
            result |= (*hexStr - 'A' + 10);
        } else if (*hexStr >= 'a' && *hexStr <= 'f') {
            result |= (*hexStr - 'a' + 10);
        } else {
            // Invalid character encountered
            return 0;
        }
        hexStr++;
    }

    return result;
}

// Converts a char* decimal number to a uint32_t
uint32_t dec_to_uint32(const char* decStr) {
    uint32_t result = 0;

    while (*decStr) {
        if (*decStr >= '0' && *decStr <= '9') {
            result = result * 10 + (*decStr - '0');
        } else {
            // Invalid character encountered
            return 0;
        }
        decStr++;
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
