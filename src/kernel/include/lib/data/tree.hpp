// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef TREE_HPP
#define TREE_HPP

#include <mm/heap.hpp>
#include <lib/data/list.hpp>

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

        tree() : root(nullptr) {}

        // Make tree move-only to avoid double frees and leaks on copies
        tree(const tree&) = delete;
        tree& operator=(const tree&) = delete;

        tree(tree&& other) noexcept : root(other.root) {
            other.root = nullptr;
        }
        tree& operator=(tree&& other) noexcept {
            if (this != &other) {
                delete_subtree(root);
                root = other.root;
                other.root = nullptr;
            }
            return *this;
        }

        ~tree() {
            // Ensure the whole tree is freed when this object goes out of scope
            delete_subtree(root);
            root = nullptr;
        }

        void set_root(Node* node) {
            // If a previous root exists, delete it to prevent leaks
            if (root && root != node) {
                delete_subtree(root);
            }
            root = node;
        }

        Node* get_root() {
            return root;
        }

        // Add and remove functions

        // Detach 'node' from its parent/sibling list
        void detach_from_parent(Node* node) {
            if (!node || !node->parent) return;

            Node* parent = node->parent;

            // If node is first child
            if (parent->first_child == node) {
                parent->first_child = node->next_sibling;
            } else {
                // Find previous sibling and unlink
                Node* prev = parent->first_child;
                while (prev && prev->next_sibling != node) {
                    prev = prev->next_sibling;
                }
                if (prev) {
                    prev->next_sibling = node->next_sibling;
                }
            }

            node->parent = nullptr;
        }

        static Node* create(const T& val) {
            Node* node = (Node*)kcalloc(1, sizeof(Node));
            if (!node) return nullptr;

            node->data = val;
            node->parent = nullptr;
            node->first_child = nullptr;
            node->next_sibling = nullptr;
            return node;
        }

        void add_child(Node* parent, Node* child) {
            if (!parent || !child) return;

            // If child already has a parent, detach it first to avoid duplicates
            if (child->parent) {
                detach_from_parent(child);
            }

            // Clear sibling pointer — avoid dragging any previous sibling chain
            child->next_sibling = nullptr;
            child->parent = parent;

            if (!parent->first_child) {
                parent->first_child = child;
            } else {
                Node* cur = parent->first_child;
                while (cur->next_sibling) cur = cur->next_sibling;
                cur->next_sibling = child;
            }
        }

        void delete_subtree(Node*& node) {
            if (!node) return;

            // First, recursively delete children
            Node* child = node->first_child;
            while (child) {
                Node* next = child->next_sibling;
                delete_subtree(child);
                child = next;
            }

            // Unlink this node from its parent so nobody keeps a dangling pointer
            if (node->parent) {
                // Reuse detach helper which will set parent->first_child or prev->next_sibling
                detach_from_parent(node);
            }

            // Extra safety: clear pointers before free
            node->first_child = nullptr;
            node->next_sibling = nullptr;
            node->parent = nullptr;

            kfree(node);
            node = nullptr;
        }

        // Helper functions

        // Gets all children
        data::list<T> get_children(Node* parent) {
            data::list<T> list = data::list<T>();

            Node* curr = parent->first_child;
            while(curr) {
                list.push_back(curr->data);
                curr = curr->next_sibling;
            }
            return list;
        }

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

        // Finds all children node that matches our specifications given by predicate
        template<typename Func>
        Node** find_children_by_predicate(Node* parent, Func predicate, int& count) {
            count = 0;
            if(!parent) return nullptr;
            
            // Counting
            Node* curr = parent->first_child;
            while(curr) {
                if(predicate(curr->data)) count++;
                curr = curr->next_sibling;
            }

            // Allocating and returning
            Node** res = (Node**)kcalloc(count, sizeof(Node));
            count = 0;
            curr = parent->first_child;
            while(curr) {
                if(predicate(curr->data)) res[count++] = curr;
                curr = curr->next_sibling;
            }
            return res;
        }

        // Traversing whole tree starting from given node
        void traverse(Node* node, void (*visit)(Node*)) {
            if (!node) return;
            
            Node* child = node->first_child;
            visit(node);
            while (child) {
                traverse(child, visit);
                child = child->next_sibling;
            }
        }

        // Traversing whole tree starting from given node
        void traverse(Node* node, void (*visit)(const Node*, int depth), int depth = 0) {
            if (!node) return;
            
            visit(node, depth);
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
