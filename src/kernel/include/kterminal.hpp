// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef KTERMINAL_HPP
#define KTERMINAL_HPP

#include <lib/data/list.hpp>
#include <lib/data/string.hpp>

typedef void (*CommandFunc)(data::list<data::string>);

#define INPUTS_TO_SAVE 10

#define INPUT_MAX_SIZE 256

// Struct that defines a command
struct Command {
    const char* name;
    const CommandFunc function;
    const char* params;
    const char* description;
};

const char* get_current_input();

namespace cmd {
    // Info from kterminal
    extern bool onTerminal;
    extern char* currentUser; // Current user using the terminal

    void init(void);
    void run_cmd(void);
    void save_cmd(void);
    void cmd_up(void);
    void cmd_down(void);
} // namespace cmd

namespace heap {
    void heap_dump(data::list<data::string> params);
}

#endif // KTERMINAL_HPP
