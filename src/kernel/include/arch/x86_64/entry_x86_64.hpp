// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef ENTRY_X86_64_HPP
#define ENTRY_X86_64_HPP

#include <stdint.h>

extern "C" void entry_x86_64(void* mbi, const uint32_t magic);

#endif // ENTRY_X86_64_HPP