// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// io.cpp
// In charge of I/O port commands
// ========================================

#include <x86/io.hpp>

namespace io {

// OutPortB and InPortB

// Sends a 8-bit value to a specified I/O port
void outPortB(const uint16_t port, const uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Receives a 8-bit value from a specific I/O port
uint8_t inPortB(const uint16_t port) {
    uint8_t value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Write a 16-bit value to an I/O port
void outPortW(const uint16_t port, const uint16_t value) {
    asm volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

// Read a 16-bit value from an I/O port
uint16_t inPortW(const uint16_t port) {
    uint16_t value;
    asm volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Writes a 32-bit value to the specified I/O port
void outPortL(const uint16_t port, const uint32_t value) {
    asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

// Reads a 32-bit value from the specified I/O port
uint32_t inPortL(const uint16_t port) {
    uint32_t value;
    asm volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

} // Namespace io
