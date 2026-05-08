// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#ifndef CONVERSIONS_HPP
#define CONVERSIONS_HPP

#include <stdint.h>
#include <lib/math.hpp>

/// @brief Converts unsigned number to string 
static const char* utoa_base(uint64_t value, int base = 10)
{
    static char buffer[65]; // Enough for 64-bit binary + null
    static const char digits[] = "0123456789ABCDEF";
    char* ptr = &buffer[64];
    *ptr = '\0';

    // Handle 0 explicitly
    if (value == 0) {
        *--ptr = '0';
        return ptr;
    }

    while (value > 0) {
        *--ptr = digits[umod64(value, base)];
        value = udiv64(value, base);
    }

    return ptr;
}

/// @brief Converts signed number to string 
static const char* itoa_base(int64_t value, int base = 10)
{
    static char buffer[66]; // one extra for sign
    bool negative = false;

    uint64_t val;
    if (value < 0) {
        negative = true;
        val = (uint64_t)(-value);
    } else {
        val = (uint64_t)value;
    }

    const char* numStr = utoa_base(val, base);

    if (negative) {
        // Prepend '-'
        char* dest = buffer;
        *dest++ = '-';
        while (*numStr)
            *dest++ = *numStr++;
        *dest = '\0';
        return buffer;
    }

    return numStr;
}

// int8_t
const char* num_to_string(int8_t num) {
    return itoa_base(num);
}

// uint8_t
const char* num_to_string(uint8_t num) {
    return utoa_base(num);
}

// int16_t
const char* num_to_string(int16_t num) {
    return itoa_base(num);
}

// uint16_t
const char* num_to_string(uint16_t num) {
    return utoa_base(num);
}

// int32_t
const char* num_to_string(int32_t num) {
    return itoa_base(num);
}

// uint32_t
const char* num_to_string(uint32_t num) {
    return utoa_base(num);
}

// int64_t
const char* num_to_string(int64_t num) {
    return itoa_base(num);
}

// uint64_t
const char* num_to_string(uint64_t num) {
    return utoa_base(num);
}

// int8_t
const char* hex_to_string(int8_t num) {
    return itoa_base(num, 16);
}

// uint8_t
const char* hex_to_string(uint8_t num) {
    return utoa_base(num, 16);
}

// int16_t
const char* hex_to_string(int16_t num) {
    return itoa_base(num, 16);
}

// uint16_t
const char* hex_to_string(uint16_t num) {
    return utoa_base(num, 16);
}

// int32_t
const char* hex_to_string(int32_t num) {
    return itoa_base(num, 16);
}

// uint32_t
const char* hex_to_string(uint32_t num) {
    return utoa_base(num, 16);
}

// int64_t
const char* hex_to_string(int64_t num) {
    return utoa_base(num, 16);
}

// uint64_t
const char* hex_to_string(uint64_t num) {
    return utoa_base(num, 16);
}


#endif // CONVERSIONS_HPP
