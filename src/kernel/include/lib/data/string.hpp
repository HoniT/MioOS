// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef STRING_HPP
#define STRING_HPP

#include <stdint.h>
#include <mm/heap.hpp>
#include <lib/string_util.hpp>
#include <lib/mem_util.hpp>
#include <drivers/vga_print.hpp>

namespace data{
    class string {
    public:
        char* data;
        uint32_t length;

        #pragma region Constructors & Destructors
        // Default constructor
        string() : data(nullptr), length(0) {}

        // Constructor from const char*
        string(const char* str) {
            // Getting length and copying data
            length = strlen(str);
            data = (char*)kmalloc(length + 1);
            memcpy(data, str, length);
            data[length] = '\0'; // Null-terminate
        }

        // Copy constructor
        string(const string& other) {
            length = other.length;
            data = (char*)kmalloc(length + 1);
            memcpy(data, other.data, length);
            data[length] = '\0'; // Null-terminate
        }

        string(const char* str, uint32_t len) {
            length = len;
            data = (char*)kmalloc(length + 1);
            memcpy(data, str, length);
            data[length] = '\0'; // Null-terminate
        }

        // Destructor
        ~string() {
            if (data) kfree(data);
        }
        #pragma endregion

        #pragma region Assignment & Conversion
        // Conversion to const char*
        operator const char*() const {
            return data;
        }

        // Assignment operator from String
        string& operator=(const string& other) {
            if (this == &other) return *this;
            if (data) kfree(data); // Freeing any unnecessary data

            // Getting length and copying data
            length = other.length;
            data = (char*)kmalloc(length + 1);
            memcpy(data, other.data, length);
            data[length] = '\0'; // Null-terminate
            return *this;
        }

        // Assignment operator from const char* (optional but useful)
        string& operator=(const char* str) {
            if (data) kfree(data); // Freeing any unnecessary data
            // Getting length and copying data
            length = strlen(str);
            data = (char*)kmalloc(length + 1);
            memcpy(data, str, length);
            data[length] = '\0'; // Null-terminate
            return *this;
        }
        #pragma endregion

        #pragma region Comparison & Access
        // Returns if this string and a given const char* are equal
        bool equals(const char* str) const {
            return strcmp(this->data, str) == 0;
        }
        // Returns if this string and a given string are equal
        bool operator==(const string& other) const {
            return strcmp(this->data, (const char*)other) == 0;
        }

        // Gets index'th member of data
        char& operator[](uint32_t index) {
            return this->data[index];
        }
        // Safer variant of operator []
        char at(uint32_t index) {
            if (index >= length) return '\0';
            return data[index];
        }

        // Returns size
        uint32_t size() const {
            return this->length;
        }
        // Returns if the string is empty
        bool empty() const {
            return this->length == 0;
        }
        #pragma endregion
    
        #pragma region Methods & Utility
        // Clearing data
        void clear() {
            // Freeing and allocating only one byte
            if(data) kfree(data);
            data = nullptr;
            length = 0;
        }
        // Appending const char* to this string
        void append(const char* str) {
            if (!str || str[0] == '\0') return; // Nothing to append

            uint32_t str_len = strlen(str);
            uint32_t new_length = length + str_len;

            // Allocate new buffer with room for null terminator
            char* new_data = (char*)kmalloc(new_length + 1);
            // Copy old data (if any)
            if (data)
                memcpy(new_data, data, length);
            // Append new string
            memcpy(new_data + length, str, str_len);
            new_data[new_length] = '\0'; // Null-terminate

            // Free old data
            if (data)
                kfree(data);
            data = new_data;
            length = new_length;
        }
        // Appending a given string to this string
        void append(const string& str) {
            append(str.data);
        }

        // Substring
        string substr(uint32_t start, uint32_t len) const {
            // If we're out of bounds
            if(start + len > length) return string();

            char* substr = (char*)kmalloc(len + 1);
            int substr_index = 0;
            for(int i = start; i < start + len; i++, substr_index++) {
                substr[substr_index] = data[i];
            }
            substr[substr_index] = '\0'; // Null-terminate

            string substr_str = substr;
            return substr_str;
        }
        #pragma endregion
    };
} // namespace data

#endif // STRING_HPP