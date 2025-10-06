// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef STRING_UTIL_HPP
#define STRING_UTIL_HPP

#include <stdint.h>
#include <stddef.h>

int32_t strcmp(const char *str1, const char *str2);
size_t strlen(const char *str);
char* strcpy(char* dest, const char* src);
char* get_first_word(const char* str);
const char* get_remaining_string(const char* str);
const char* get_word_at_index(const char* str, int index);
uint32_t get_words(const char* str);
int str_to_int(const char* str);
char* strcat(char* dest, const char* src);

const char* num_to_string(int8_t num);
const char* num_to_string(uint8_t num);
const char* num_to_string(int16_t num);
const char* num_to_string(uint16_t num);
const char* num_to_string(int32_t num);
const char* num_to_string(uint32_t num);
const char* num_to_string(int64_t num);
const char* num_to_string(uint64_t num);

const char* hex_to_string(int8_t num);
const char* hex_to_string(uint8_t num);
const char* hex_to_string(int16_t num);
const char* hex_to_string(uint16_t num);
const char* hex_to_string(int32_t num);
const char* hex_to_string(uint32_t num);
const char* hex_to_string(int64_t num);
const char* hex_to_string(uint64_t num);

#endif // STRING_UTIL_HPP
