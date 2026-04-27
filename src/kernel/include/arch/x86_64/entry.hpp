// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef ENTRY_HPP
#define ENTRY_HPP

#include <stdint.h>

[[noreturn]] void kpanic(const char* msg) noexcept;
void klog(const char* msg) noexcept;
void klog_hex(uint64_t num) noexcept;
void klog_num(int64_t num) noexcept;

extern "C" void entry_x86_64(void* mbi, const uint32_t magic);

#endif // ENTRY_HPP