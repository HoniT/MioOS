// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef STORAGE_CLI_HPP
#define STORAGE_CLI_HPP

#include <apps/cli_app.hpp>

namespace cmd {
    /// @brief Storage/File system CLI commands
    class storage_cli : public cli_app {
    public:
        static void register_app();

        static void read_ata();
        static void list_ata();
        static void pwd();
        static void ls();
        static void cd();
        static void mkdir();
        static void mkfile();
        static void rm();
        static void cat();
        static void write_to_file();
        static void append_to_file();
    };
}

#endif // STORAGE_CLI_HPP
