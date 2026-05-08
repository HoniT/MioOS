// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef IO_HPP
#define IO_HPP

#include <stdint.h>

namespace cpu
{
    /// @brief Sends a byte of data to a specified I/O port
    void outb(const uint16_t port, const uint8_t value);
    /// @brief Reads a byte of data from a specified I/O port
    uint8_t inb(const uint16_t port);
    
    /// @brief Sends a word of data to a specified I/O port
    void outw(const uint16_t port, const uint16_t value);
    /// @brief Reads a word of data from a specified I/O port
    uint16_t inw(const uint16_t port);
    
    /// @brief Sends a dword of data to a specified I/O port
    void outl(const uint16_t port, const uint32_t value);
    /// @brief Reads a dword of data from a specified I/O port
    uint32_t inl(const uint16_t port);
} // namespace cpu


#endif // IO_HPP
