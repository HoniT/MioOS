// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef LARGE_STRING_HPP
#define LARGE_STRING_HPP

#include <stdint.h>
#include <mm/pmm.hpp>
#include <lib/string_util.hpp>
#include <lib/mem_util.hpp>
#include <lib/data/list.hpp>

namespace data {
    /// @brief String that allocates from the PMM instead of the heap.
    /// Suitable for large, persistent strings.
    class large_string {
    private:
        char* data;
        uint32_t length;
        uint32_t blocks_used;

        static inline uint32_t calc_blocks(uint32_t bytes) {
            return (bytes + FRAME_SIZE - 1) / FRAME_SIZE; // round up to full frames
        }

    public:
        #pragma region Constructors & Destructors
        large_string() : data(nullptr), length(0), blocks_used(0) {}

        large_string(const char* str) {
            if (!str) {
                data = nullptr;
                length = 0;
                blocks_used = 0;
                return;
            }

            length = strlen(str);
            blocks_used = calc_blocks(length + 1);
            data = (char*)pmm::alloc_frame(blocks_used);
            memcpy(data, str, length);
            data[length] = '\0';
        }

        large_string(const char* str, uint32_t len) {
            length = len;
            blocks_used = calc_blocks(length + 1);
            data = (char*)pmm::alloc_frame(blocks_used);
            memcpy(data, str, length);
            data[length] = '\0';
        }

        large_string(const large_string& other) {
            length = other.length;
            blocks_used = calc_blocks(length + 1);
            data = (char*)pmm::alloc_frame(blocks_used);
            memcpy(data, other.data, length);
            data[length] = '\0';
        }

        ~large_string() {
            if (data)
                pmm::free_frame(data);
        }
        #pragma endregion

        #pragma region Assignment & Conversion
        operator const char*() const { return data; }

        large_string& operator=(const large_string& other) {
            if (this == &other) return *this;

            if (data)
                pmm::free_frame(data);

            length = other.length;
            blocks_used = calc_blocks(length + 1);
            data = (char*)pmm::alloc_frame(blocks_used);
            memcpy(data, other.data, length);
            data[length] = '\0';
            return *this;
        }

        large_string& operator=(const char* str) {
            if (!str) return *this;

            if (data)
                pmm::free_frame(data);

            length = strlen(str);
            blocks_used = calc_blocks(length + 1);
            data = (char*)pmm::alloc_frame(blocks_used);
            memcpy(data, str, length);
            data[length] = '\0';
            return *this;
        }
        #pragma endregion

        #pragma region Comparison & Access
        bool equals(const char* str) const {
            return strcmp(this->data, str) == 0;
        }

        bool operator==(const large_string& other) const {
            return strcmp(this->data, other.data) == 0;
        }

        bool operator==(const char* other) const {
            return strcmp(this->data, other) == 0;
        }

        bool operator!=(const large_string& other) const {
            return strcmp(this->data, other.data) != 0;
        }

        bool operator!=(const char* other) const {
            return strcmp(this->data, other) != 0;
        }

        char& operator[](uint32_t index) { return this->data[index]; }

        char at(uint32_t index) const {
            if (index >= length) return '\0';
            return data[index];
        }

        uint32_t size() const { return length; }
        bool empty() const { return length == 0; }
        #pragma endregion

        #pragma region Methods & Utility
        void clear() {
            if (data) {
                pmm::free_frame(data);
                data = nullptr;
            }
            length = 0;
            blocks_used = 0;
        }

        large_string& append(const char* str) {
            if (!str || !*str) return *this;

            uint32_t str_len = strlen(str);
            uint32_t new_length = length + str_len;

            uint32_t new_blocks = calc_blocks(new_length + 1);
            char* new_data = (char*)pmm::alloc_frame(new_blocks);

            if (data)
                memcpy(new_data, data, length);
            memcpy(new_data + length, str, str_len);
            new_data[new_length] = '\0';

            if (data)
                pmm::free_frame(data);

            data = new_data;
            length = new_length;
            blocks_used = new_blocks;

            return *this;
        }

        large_string& append(const large_string& str) {
            return append(str.data);
        }

        large_string& append(const char c) {
            char temp[2] = { c, '\0' };
            return append(temp);
        }

        const char* c_str() const { return data; }

        large_string substr(uint32_t start, uint32_t len) const {
            if (start >= length) return large_string();
            if (start + len > length)
                len = length - start;

            uint32_t blocks_needed = calc_blocks(len + 1);
            char* temp = (char*)pmm::alloc_frame(blocks_needed);
            memcpy(temp, data + start, len);
            temp[len] = '\0';

            large_string result(temp, len);
            pmm::free_frame(temp);
            return result;
        }

        bool includes(const char* substr) const {
            if (!substr) return false;

            for (uint32_t i = 0; i < length; i++) {
                uint32_t j = 0;
                while (substr[j] && data[i + j] && data[i + j] == substr[j])
                    j++;
                if (!substr[j]) return true;
            }
            return false;
        }
        #pragma endregion
    };
} // namespace data

data::list<data::string> split_string_tokens(data::string str);

#endif // LARGE_STRING_HPP
