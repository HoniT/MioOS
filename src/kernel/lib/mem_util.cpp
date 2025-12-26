// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// mem_util.cpp
// In charge of keeping memory utility functions
// ========================================

#include <lib/mem_util.hpp>
#include <lib/math.hpp>
#include <lib/data/string.hpp>

/// @brief Splits bytes into different units
/// @return String representing units (e.g. 1_049_601 B = 1MiB 1KiB 1B)
data::string get_units(uint64_t bytes) {
    if (bytes == 0) return "0 B";

    const char* suffix = "B";
    uint64_t divisor = 1;

    // Pick the largest unit
    if (bytes >= BYTES_IN_GIB) {
        suffix = "GiB";
        divisor = BYTES_IN_GIB;
    } else if (bytes >= BYTES_IN_MIB) {
        suffix = "MiB";
        divisor = BYTES_IN_MIB;
    } else if (bytes >= BYTES_IN_KIB) {
        suffix = "KiB";
        divisor = BYTES_IN_KIB;
    } else {
        // For plain Bytes, we don't need decimals
        data::string result = num_to_string(bytes);
        result.append(" B");
        return result;
    }

    // Calculate Whole Number part
    uint64_t whole = udiv64(bytes, divisor);

    // Calculate Fractional part (1 decimal place)
    uint64_t remainder = umod64(bytes, divisor);
    uint64_t fraction = udiv64(remainder * 10, divisor);

    // Construct the string: "Whole.Fraction Suffix"
    data::string result;
    result.append(num_to_string(whole));
    result.append(".");
    result.append(num_to_string(fraction));
    result.append(" ");
    result.append(suffix);

    return result;
}

// Memset sets a block of memory to a specific value for a given number of bytes
void memset(const void *dest, const char val, uint32_t count){
    char *temp = (char*) dest;
    for (; count != 0; count--){
        *temp++ = val;
    }

}

// Memcpy copies a n sized source to a destination
void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

uint32_t memcmp(const void *s1, const void *s2, const size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (int)p1[i] - (int)p2[i];
        }
    }
    return 0;
}

// Alligns the size to the allignment
size_t align_up(const size_t size, const size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}
