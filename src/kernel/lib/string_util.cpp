// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// string_util.cpp
// In charge of keeping string utility functions
// ========================================

#include <lib/string_util.hpp>
#include <lib/data/string.hpp>
#include <lib/math.hpp>
#include <lib/data/list.hpp>

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

// Gets the remainder of a string (everything except the first word)
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

const char* get_word_at_index(const char* str, int index) {
    int current_index = 0;
    int i = 0;

    // Buffer for the word
    static char word[256];

    while (str[i] != '\0') {
        // Skip leading whitespace
        while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
            i++;
        }

        // If end of string after whitespace, break
        if (str[i] == '\0') break;

        // Start of a word
        int j = 0;
        int start = i;

        while (str[i] != '\0' && str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
            if (j < sizeof(word) - 1) {
                word[j++] = str[i];
            }
            i++;
        }

        word[j] = '\0'; // Null-terminate the word

        if (current_index == index) {
            return word;
        }

        current_index++;
    }

    return nullptr;
}

uint32_t get_words(const char* str) {
    int count = 0;
    int i = 0;

    while (str[i] != '\0') {
        // Skip any whitespace
        while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
            i++;
        }

        // If a word starts here, count it
        if (str[i] != '\0') {
            count++;

            // Skip this word
            while (str[i] != '\0' && str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
                i++;
            }
        }
    }

    return count;
}


// Turns a char* into an int
int str_to_int(const char* str) {
    if (!str || *str == '\0') return -1; // Null or empty string

    int result = 0;

    while (*str) {
        if (*str < '0' || *str > '9') {
            return -1; // Illegal (non-digit) character
        }

        result = result * 10 + (*str - '0');
        str++;
    }

    return result;
}


// Copies a source string to a destination
char* strcpy(char* dest, const char* src) {
    char* dest_ptr = dest; // Save the starting address of destination

    // Copy each character from source to destination
    while ((*dest_ptr++ = *src++));

    return dest; // Return the destination pointer
}

// Appends src to the end of dest
char* strcat(char* dest, const char* src) {
    if (!dest || !src) return dest;

    // Move to the end of dest
    char* d = dest;
    while (*d) d++;

    // Copy src to dest
    while (*src) {
        *d++ = *src++;
    }

    *d = '\0'; // Null-terminate
    return dest;
}

// Splits a string into tokens
data::list<data::string> split_string_tokens(data::string str) {
    data::list<data::string> tokens;
    if (str.empty()) {
        return tokens;
    }
    uint32_t i = 0;
    while (i < str.size()) {
        // Skip repeated slashes
        while (i < str.size() && str[i] == ' ') i++;

        uint32_t start = i;
        while (i < str.size() && str[i] != ' ') i++;
        uint32_t len = i - start;

        if (len > 0) {
            tokens.push_back(data::string(str.c_str() + start, len));
        }
    }
    return tokens;
}

// Helper: convert unsigned number to string (decimal)
static const char* utoa_base(uint64_t value, int base = 10)
{
    static char buffer[65]; // Enough for 64-bit binary + null
    static const char digits[] = "0123456789ABCDEF";
    char* ptr = &buffer[64];
    *ptr = '\0';

    // Handle 0 explicitly
    if (value == 0) {
        *--ptr = '0';
        return ptr;
    }

    while (value > 0) {
        *--ptr = digits[umod64(value, base)];
        value = udiv64(value, base);
    }

    return ptr;
}

// Helper: convert signed number to string (decimal)
static const char* itoa_base(int64_t value, int base = 10)
{
    static char buffer[66]; // one extra for sign
    bool negative = false;

    uint64_t val;
    if (value < 0) {
        negative = true;
        val = (uint64_t)(-value);
    } else {
        val = (uint64_t)value;
    }

    const char* numStr = utoa_base(val, base);

    if (negative) {
        // Prepend '-'
        char* dest = buffer;
        *dest++ = '-';
        while (*numStr)
            *dest++ = *numStr++;
        *dest = '\0';
        return buffer;
    }

    return numStr;
}

// ========== Overloads ==========

// int8_t
const char* num_to_string(int8_t num) {
    return itoa_base(num);
}

// uint8_t
const char* num_to_string(uint8_t num) {
    return utoa_base(num);
}

// int16_t
const char* num_to_string(int16_t num) {
    return itoa_base(num);
}

// uint16_t
const char* num_to_string(uint16_t num) {
    return utoa_base(num);
}

// int32_t
const char* num_to_string(int32_t num) {
    return itoa_base(num);
}

// uint32_t
const char* num_to_string(uint32_t num) {
    return utoa_base(num);
}

// int64_t
const char* num_to_string(int64_t num) {
    return itoa_base(num);
}

// uint64_t
const char* num_to_string(uint64_t num) {
    return utoa_base(num);
}

// int8_t
const char* hex_to_string(int8_t num) {
    return itoa_base(num, 16);
}

// uint8_t
const char* hex_to_string(uint8_t num) {
    return utoa_base(num, 16);
}

// int16_t
const char* hex_to_string(int16_t num) {
    return itoa_base(num, 16);
}

// uint16_t
const char* hex_to_string(uint16_t num) {
    return utoa_base(num, 16);
}

// int32_t
const char* hex_to_string(int32_t num) {
    return itoa_base(num, 16);
}

// uint32_t
const char* hex_to_string(uint32_t num) {
    return utoa_base(num, 16);
}

// int64_t
const char* hex_to_string(int64_t num) {
    return utoa_base(num, 16);
}

// uint64_t
const char* hex_to_string(uint64_t num) {
    return utoa_base(num, 16);
}
