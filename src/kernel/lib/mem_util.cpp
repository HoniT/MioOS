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
    if (bytes == 0) return "0B";

    data::string result;
    uint64_t remaining = bytes;

    // Calculate each unit
    size_t g_count = udiv64(remaining, BYTES_IN_GIB);
    remaining = umod64(remaining, BYTES_IN_GIB);

    size_t m_count = udiv64(remaining, BYTES_IN_MIB);
    remaining = umod64(remaining, BYTES_IN_MIB);

    size_t k_count = udiv64(remaining, BYTES_IN_KIB);
    remaining = umod64(remaining, BYTES_IN_KIB);

    size_t b_count = remaining;

    // Append to string if the unit exists
    if (g_count > 0) {
        result.append(num_to_string(g_count));
        result.append("GiB ");
    }
    if (m_count > 0) {
        result.append(num_to_string(m_count));
        result.append("MiB ");
    }
    if (k_count > 0) {
        result.append(num_to_string(k_count));
        result.append("KiB ");
    }
    if (b_count > 0 || result.size() == 0) {
        result.append(num_to_string(b_count));
        result.append("B");
    }

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
