// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef KERNEL_PANIC_HPP
#define KERNEL_PANIC_HPP

#include <interrupts/idt.hpp>

// Kernel panic functions
void kernel_panic(const char* error);
void kernel_panic(const char* error, InterruptRegistersISR* frame);

#endif // KERNEL_PANIC_HPP
