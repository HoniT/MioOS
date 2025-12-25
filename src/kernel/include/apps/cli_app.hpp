// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef CLI_APP_HPP
#define CLI_APP_HPP

#include <lib/data/list.hpp>
#include <lib/data/string.hpp>

namespace cmd {
    /// @brief Argument getting strategies
    enum class ArgStrategy {
        Tokenize,       // Standard: ["arg1", "arg2", "arg3"]
        RawRest,        // Whole tail: ["arg1 arg2 arg3"]
        SplitHead       // First separate, rest raw: ["arg1", "arg2 arg3"]
    };

    /// @brief Base class to define helper functions for CLI applicaitons
    class cli_app {
    private:
        static data::list<data::string> get_params_tokenized();
        static data::list<data::string> get_params_raw_rest();
        static data::list<data::string> get_params_split_head();
    public:
        static data::list<data::string> get_params(ArgStrategy argStrategy = ArgStrategy::Tokenize);
    };
}

#endif // CLI_APP_HPP
