// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// string_util.cpp
// In charge of keeping string utility functions
// ========================================

#include <lib/string_util.hpp>

// Compares to strings together and returns the difference
int32_t strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

// Returns a strings length
size_t strlen(const char *str) {
    size_t length = 0;
    while (str[length] != '\0') {
        length++;
    }
    return length;
}

// Gets the first word in a sting
char* get_first_word(const char* str) {
    static char first_word[256]; // Buffer for the first word, adjust size as needed
    int i = 0;

    // Skip leading whitespace
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
        i++;
    }

    // Copy characters of the first word
    int j = 0;
    while (str[i] != '\0' && str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
        first_word[j++] = str[i++];
    }

    first_word[j] = '\0'; // Null-terminate the first word
    return first_word;
}

// gets the remainder of a string (everything except the first word)
const char* get_remaining_string(const char* str) {
    static char rest[1024]; // Buffer for the rest of the string, adjust size as needed
    int i = 0;

    // Skip leading whitespace
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
        i++;
    }

    // Skip the first word
    while (str[i] != '\0' && str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
        i++;
    }

    // Skip whitespace between the first word and the rest of the string
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
        i++;
    }

    // Copy the rest of the string
    int j = 0;
    while (str[i] != '\0') {
        rest[j++] = str[i++];
    }

    rest[j] = '\0'; // Null-terminate the rest of the string
    return rest;
}

// Copies a source string to a destination
char* strcpy(char* dest, const char* src) {
    char* dest_ptr = dest; // Save the starting address of destination

    // Copy each character from source to destination
    while ((*dest_ptr++ = *src++));

    return dest; // Return the destination pointer
}
