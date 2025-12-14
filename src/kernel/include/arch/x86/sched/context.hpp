// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <stdint.h>

// CPU context that needs to be saved/restored
struct context_t {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds, es, fs, gs, ss;
    uint32_t cr3;
};

extern "C" [[noreturn]] void ctx_switch(context_t* old_ctx, context_t* new_ctx);

#endif // CONTEXT_HPP
