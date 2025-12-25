// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// cli_app.cpp
// Base class for CLI application helper functions
// ========================================

#include <apps/cli_app.hpp>
#include <apps/kterminal.hpp>

/// @brief Returns tokenized parameters
data::list<data::string> cmd::cli_app::get_params_tokenized() {
    data::string raw_input = get_remaining_string(cmd::get_input());
    return split_string_tokens(raw_input);
}

/// @brief Returns raw rest parameters
data::list<data::string> cmd::cli_app::get_params_raw_rest() {
    data::list<data::string> list = data::list<data::string>();
    list.add(get_remaining_string(cmd::get_input()));
    return list;
}

/// @brief Returns split head parameters
data::list<data::string> cmd::cli_app::get_params_split_head() {
    data::list<data::string> list = data::list<data::string>();
    list.add(get_first_word(get_remaining_string(cmd::get_input()))); // First arg
    list.add(get_remaining_string(cmd::get_input())); // Rest of args
    return list;
}

/// @brief Takes kterminal input and returns passed parameters
/// @param argStrategy Argument getting startegy
/// @return List of arguments passed
data::list<data::string> cmd::cli_app::get_params(cmd::ArgStrategy argStrategy) {
    switch (argStrategy) {
        case ArgStrategy::Tokenize:
            return get_params_tokenized();
        case ArgStrategy::RawRest:
            return get_params_raw_rest();
        case ArgStrategy::SplitHead:
            return get_params_split_head();
    };
}
