// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#ifndef MATH_UTIL_HPP
#define MATH_UTIL_HPP

#include  <stdint.h>

uint64_t udiv64(uint64_t dividend, uint64_t divisor) {
    if (divisor == 0) {
        // Handles division by zero
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

uint64_t umod64(uint64_t dividend, uint64_t divisor) {
    if (divisor == 0) {
        // Handles division by zero
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

#endif // MATH_UTIL_HPP