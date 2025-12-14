// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef KERNEL_MAIN_HPP
#define KERNEL_MAIN_HPP

#include <stdint.h>

// MioOS kernel version
extern const char* kernel_version;

extern "C" void kernel_main(const uint32_t magic, void* mbi);

#endif // KERNEL_MAIN_HPP
