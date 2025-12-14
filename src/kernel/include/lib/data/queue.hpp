// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <mm/heap.hpp>

namespace data {

    template<typename T>
    class queue {
    private:
        struct node {
            T value;
            node* next;
        };

        node* head;
        node* tail;

        int count;

    public:
        queue() : head(nullptr), tail(nullptr) {}

        queue(const queue<T>& other) : head(nullptr), tail(nullptr), count(0) {
            node* cur = other.head;
            while (cur) {
                push(cur->value);
                cur = cur->next;
            }
        }

        ~queue() {
            node* curr = head;
            while(curr) {
                node* next = curr->next;
                kfree(curr);
                curr = next;
            }

            if(head) kfree(head);
            if(tail) kfree(tail);
        }

        /// @brief Returns if queue is empty
        bool empty() const {
            return head == nullptr;
        }

        /// @brief Returns queue size
        uint32_t size() {
            return count;
        }

        /// @brief Adds a value to the queue
        void push(const T& value) {
            node* n = (node*) kmalloc(sizeof(node));
            n->value = value;
            n->next = nullptr;

            if (!tail) {
                head = tail = n;
            } else {
                tail->next = n;
                tail = n;
            }
            count++;
        }

        /// @brief Removes and returns first queue element
        T pop() {
            if (!head) return T();

            node* n = head;
            T val = n->value;
            head = head->next;

            if (!head)
                tail = nullptr; // Queue empty now

            kfree(n);
            count--;
            return val;
        }

        /// @brief Pops and repushes the value
        T requeue() {
            T val = this->pop();
            if(val) this->push(val);
            return val;
        }

        /// @brief Returns first queue element
        T& front() {
            return head->value;
        }
    };
} // namespace data

#endif // QUEUE_HPP
