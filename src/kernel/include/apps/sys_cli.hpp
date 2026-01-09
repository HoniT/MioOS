// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef SYS_CLI_HPP
#define SYS_CLI_HPP

#include <apps/cli_app.hpp>

namespace cmd {
    /// @brief System info CLI commands
    class sys_cli : public cli_app {
    public:
        static void register_app();

        static void sysinfo();
        static void uptime();
        static void currtime();
        static void lsprocesses();
    };
}

#endif // SYS_CLI_HPP
