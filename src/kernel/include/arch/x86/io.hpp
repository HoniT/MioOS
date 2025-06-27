// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef IO_HPP
#define IO_HPP

#include <stdint.h>

namespace io {

// OutPort and InPort

// Byte
void outPortB(const uint16_t port, const uint8_t value);
uint8_t inPortB(const uint16_t port);
// Word
void outPortW(const uint16_t port, const uint16_t value);
uint16_t inPortW(const uint16_t port);
// DWord
void outPortL(const uint16_t port, const uint32_t value);
uint32_t inPortL(const uint16_t port);

} // Namespace io

#endif // IO_HPP
