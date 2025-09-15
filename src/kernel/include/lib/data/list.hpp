// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef LIST_HPP
#define LIST_HPP

#include <stdint.h>
#include <mm/heap.hpp>
#include <lib/mem_util.hpp>
#include <drivers/vga_print.hpp>

namespace data {

    template<typename T>
    class list {
    private:
        T* arr;
        uint32_t length;     // Number of elements
        uint32_t capacity;   // Allocated size

        /// @brief Resizes array to fit new capacity
        /// @param new_capacity New capacity
        void resize(uint32_t new_capacity) {
            if (new_capacity < length) new_capacity = length;

            T* new_arr = (T*)kmalloc(sizeof(T) * new_capacity);

            // Copy old data
            if (arr) {
                memcpy(new_arr, arr, sizeof(T) * length);
                kfree(arr);
            }

            arr = new_arr;
            capacity = new_capacity;
        }

    public:
        #pragma region Constructors & Destructor
        list() : arr(nullptr), length(0), capacity(0) {}

        // Construct from pointer + length
        list(const T* src, uint32_t len) {
            length = len;
            capacity = len;
            arr = (T*)kmalloc(sizeof(T) * capacity);
            memcpy(arr, src, sizeof(T) * len);
        }

        ~list() {
            if (arr) kfree(arr);
        }
        #pragma endregion

        #pragma region Capacity
        /// @brief Returns amount of elements in list
        uint32_t count() const { return length; }

        /// @brief Returns if the arra is empty 
        bool empty() const { return length == 0; }
        #pragma endregion

        #pragma region Element Access
        T& operator[](uint32_t index) {
            return arr[index];
        }

        const T& operator[](uint32_t index) const {
            return arr[index];
        }

        /// @brief Safe way to get element at an index 
        T& at(uint32_t index) {
            // Bounds checking
            if (index >= length) {
                vga::error("Index (%u) out of bounds for list!\n", index);
                return arr[0]; // fallback
            }
            return arr[index];
        }

        /// @brief Returns first element 
        T& front() { return arr[0]; }

        /// @brief Returns last element 
        T& back() { return arr[length - 1]; }
        #pragma endregion

        #pragma region Modifiers
        /// @brief Adds a value to list
        void push_back(const T& value) {
            if (length >= capacity) {
                uint32_t new_capacity = (capacity == 0) ? 2 : capacity * 2;
                resize(new_capacity);
            }
            arr[length++] = value;
        }

        /// @brief Removes last element from list
        void pop_back() {
            if (length > 0) {
                length--;
            }
        }

        /// @brief Clears list
        void clear() {
            if (arr) kfree(arr);
            arr = nullptr;
            length = 0;
            capacity = 0;
        }

        /// @brief Inserts value at an index 
        void insert(uint32_t index, const T& value) {
            if (index > length) index = length; // Clamp

            if (length >= capacity) {
                uint32_t new_capacity = (capacity == 0) ? 2 : capacity * 2;
                resize(new_capacity);
            }

            // Shift elements to the right
            for (uint32_t i = length; i > index; i--) {
                arr[i] = arr[i - 1];
            }

            arr[index] = value;
            length++;
        }

        /// @brief Erases the value found on an index 
        void erase(uint32_t index) {
            if (index >= length) return;

            // Shift left
            for (uint32_t i = index; i < length - 1; i++) {
                arr[i] = arr[i + 1];
            }

            length--;
        }
        #pragma endregion

        #pragma region Iteration
        /// @brief Returns array starting from the lists begining 
        T* begin() { return arr; }

        /// @brief Returns array starting from the lists end 
        T* end() { return arr + length; }

        /// @brief Returns array starting from the lists begining 
        const T* begin() const { return arr; }

        /// @brief Returns array starting from the lists end 
        const T* end() const { return arr + length; }
        #pragma endregion
    };

} // namespace data

#endif // LIST_HPP
