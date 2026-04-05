// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef KERNEL_MAIN_HPP
#define KERNEL_MAIN_HPP

#include <stdint.h>

// MioOS kernel version
extern const char* kernel_version;

extern "C" void kernel_main(void* mbi, const uint32_t magic);

#endif // KERNEL_MAIN_HPP