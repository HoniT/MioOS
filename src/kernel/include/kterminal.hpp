// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef KTERMINAL_HPP
#define KTERMINAL_HPP

// External symbols
extern bool onTerminal;
extern char* currentInput;

typedef void (*CommandFunc)();

// Struct that defines a command
struct Command {
    const char* name;
    const CommandFunc function;
    const char* description;
};

namespace cmd {
    void init(void);
    void run_cmd(void);
} // namespace cmd


#endif // KTERMINAL_HPP
