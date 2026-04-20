// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VMA_HPP
#define VMA_HPP

#include <stdint.h>
#include <stddef.h>

namespace mem {
    enum VMAType {
        VMA_HEAP,
        VMA_STACK,
        VMA_MMAP,
        VMA_ANONYMOUS
    };

    /// Architecture-independent permission flags
    struct PageFlags {
        bool read;
        bool write;
        bool execute;
        bool user;
        bool uncacheable; 
        bool copy_on_write;
    };

    struct VMArea {
        uint64_t start;
        uint64_t end;
        PageFlags flags;
        VMAType type;

        // Tree nodes (For Red-Black Tree or AVL Tree)
        VMArea* left;
        VMArea* right;
        VMArea* parent;
        bool is_red; 
    };

    class VMATree {
    private:
        VMArea* root;
        void insert_fixup(VMArea* node);
        void delete_fixup(VMArea* node);

    public:
        VMATree() : root(nullptr) {}
        ~VMATree();

        VMArea* insert(uintptr_t start, size_t size, PageFlags flags, VMAType type);
        void remove(uintptr_t start, size_t size);
        VMArea* find(uintptr_t address);
        bool is_range_free(uintptr_t start, size_t size);
    };
} // namespace mem

#endif // VMA_HPP
