// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef MEM_CLI_HPP
#define MEM_CLI_HPP

#include <apps/cli_app.hpp>

namespace cmd {
    /// @brief Memory CLI commands
    class mem_cli : public cli_app {
    public:
        static void register_app();

        static void meminfo();
        static void heapdump();
    };
}

#endif // MEM_CLI_HPP
