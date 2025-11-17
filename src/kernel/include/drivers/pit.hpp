// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef PIT_HPP
#define PIT_HPP

#include <stdint.h>
#include <lib/data/list.hpp>
#include <lib/data/string.hpp>

extern volatile uint64_t ticks;
extern const uint32_t frequency;

namespace pit {
    void init(void); // Initializes the PIT
    void delay(const uint64_t ms);

    // Terminal functions
    void getuptime(data::list<data::string> params);
} // Namespace pit

#endif // PIT_HPP
