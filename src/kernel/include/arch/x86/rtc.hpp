// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef RTC_HPP
#define RTC_HPP

#include <stdint.h>
#include <lib/data/string.hpp>

#define RTC_PORT 0x70

namespace rtc {
    uint8_t get_year();
    uint8_t get_month();
    uint8_t get_day();
    uint8_t get_weekday();
    uint8_t get_hour();
    uint8_t get_minute();
    uint8_t get_second();

    inline uint8_t bcd_to_bin(uint8_t val);

    uint32_t get_unix_timestamp();
    data::string timestamp_to_string(uint32_t ts);
    void print_time(void);
}

#endif // RTC_HPP
