// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef TREE_HPP
#define TREE_HPP

#include <mm/heap.hpp>

namespace data {

    template <typename T>
    class tree {
    public:
        struct Node {
            // Custom template for data
            T data;

            Node* parent;
            Node* first_child;
            Node* next_sibling;
        };

        tree() : root(nullptr) {};
        void set_root(Node* node) {
            root = node;
        }
        Node* get_root() {
            return root;
        }

        // Add and remove functions

        Node* create(const T& val) {
            // Allocating node
            Node* node = (Node*)kmalloc(sizeof(Node));
            if(!node) return nullptr;

            // Setting values
            node->data = val;
            node->parent = nullptr;
            node->first_child = nullptr;
            node->next_sibling = nullptr;

            return node;
        }

        void add_child(Node* parent, Node* child) {
            if(!parent || !child) return;

            child->parent = parent;
            // If parent hasn't got a child adding the child as first child
            if(!parent->first_child) {
                parent->first_child = child;
            }
            // Else we'll add it to the end of the children list
            else {
                Node* cur = parent->first_child;
                while (cur->next_sibling)
                    cur = cur->next_sibling;
                cur->next_sibling = child;
            }
        }

        void delete_subtree(Node* node) {
            if(!node) return;

            // Deleting subtrees recursively
            Node* child = node->first_child;
            while (child) {
                Node* next = child->next_sibling;
                delete_subtree(child);
                child = next;
            }

            // Freeing memory
            kfree(node);
        }

        // Helper functions

        // Finds a child node using predicate
        template<typename Func>
        Node* find_child_by_predicate(Node* parent, Func predicate) {
            if (!parent) return nullptr;

            // Traversing untill we find the child
            Node* cur = parent->first_child;
            while (cur) {
                if (predicate(cur->data)) return cur;
                cur = cur->next_sibling;
            }
            return nullptr;
        }

        // Traversing whole tree starting from given node
        void traverse(Node* node, void (*visit)(const T&, int depth), int depth = 0) {
            if (!node) return;
            
            visit(node->data, depth);
            Node* child = node->first_child;
            while (child) {
                traverse(child, visit, depth + 1);
                child = child->next_sibling;
            }
        }

    private:
        Node* root;
    };

} // namespace data

#endif // TREE_HPP
