// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef KERNEL_PANIC_HPP
#define KERNEL_PANIC_HPP

/// @brief Preforms kernel panic
/// @param msg Displayed message
void kernel_panic(const char* msg, ...);

#endif // KERNEL_PANIC_HPP
