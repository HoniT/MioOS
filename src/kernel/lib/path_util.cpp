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

// Splits a file/dir path into tokens (returns every dir/file going into the path in an array given by the user and stores count)
char** split_path_tokens(const char* path, int& count) {
    int max_tokens = 16;
    char** tokens = (char**)kmalloc(sizeof(char*) * max_tokens);

    int token_index = 0;

    if (path[0] == '/') {
        tokens[token_index] = (char*)kmalloc(1);
        strcpy(tokens[token_index], "/");
        token_index++;
        path++;
    }

    while (*path && token_index < max_tokens) {
        // Skip repeated '/'
        while (*path == '/') path++;

        // Allocate and extract token
        char* token = (char*)kmalloc(256);
        int i = 0;
        while (*path && *path != '/' && i < 255) {
            token[i++] = *path++;
        }
        token[i] = '\0';

        if (i > 0) {
            tokens[token_index++] = token;
        } else {
            kfree(token); // No useful data, discard
        }
    }

    count = token_index;
    return tokens;
}