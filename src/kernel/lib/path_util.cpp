// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// path_util.cpp
// Ext2 path utility functions
// ========================================

#include <lib/path_util.hpp>
#include <mm/heap.hpp>
#include <lib/string_util.hpp>
#include <lib/data/string.hpp>
#include <drivers/vga_print.hpp>

// Splits a file/dir path into tokens (returns every dir/file going into the path in an array given by the user and stores count)
data::string* split_path_tokens(data::string path, int& count) {
    if (path.empty()) return nullptr;


    int max_tokens = 16;
    data::string* tokens = (data::string*)kmalloc(sizeof(data::string) * max_tokens);

    int token_index = 0;
    uint32_t i = 0;

    // Add leading slash as a token if present
    if (path.at(0) == '/') {
        tokens[token_index++] = data::string("/");
        i++; // Skip the first '/'
    }

    while (i < path.size() && token_index < max_tokens) {
        // Skip repeated slashes
        while (i < path.size() && path[i] == '/') i++;

        // Start of token
        uint32_t start = i;
        while (i < path.size() && path[i] != '/') i++;
        uint32_t len = i - start;

        if (len > 0) {
            // Create string from substring
            tokens[token_index++] = data::string(path.data + start, len);
        }
    }

    count = token_index;
    return tokens;
}
