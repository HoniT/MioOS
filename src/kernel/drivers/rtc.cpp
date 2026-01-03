// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// rtc.cpp
// Sets up the Real Time Clock
// ========================================

#include <drivers/rtc.hpp>
#include <x86/io.hpp>
#include <graphics/vga_print.hpp>

uint8_t rtc::get_year()
{
	io::outPortB(RTC_PORT, 0x09);
	return rtc::bcd_to_bin(io::inPortB(0x71));
}

uint8_t rtc::get_month()
{
	io::outPortB(RTC_PORT, 0x08);
	return rtc::bcd_to_bin(io::inPortB(0x71));
}

uint8_t rtc::get_day()
{
	io::outPortB(RTC_PORT, 0x07);
	return rtc::bcd_to_bin(io::inPortB(0x71));
}

uint8_t rtc::get_weekday()
{
	io::outPortB(RTC_PORT, 0x06);
	return rtc::bcd_to_bin(io::inPortB(0x71));
}

uint8_t rtc::get_hour()
{
	io::outPortB(RTC_PORT, 0x04);
	return rtc::bcd_to_bin(io::inPortB(0x71));
}

uint8_t rtc::get_minute()
{
	io::outPortB(RTC_PORT, 0x02);
	return rtc::bcd_to_bin(io::inPortB(0x71));
}

uint8_t rtc::get_second()
{
	io::outPortB(RTC_PORT, 0x00);
	return rtc::bcd_to_bin(io::inPortB(0x71));
}

inline uint8_t rtc::bcd_to_bin(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

// Gets UNIX timestamp time 
uint32_t rtc::get_unix_timestamp() {
    // Read CMOS values
    uint8_t sec = get_second();
    uint8_t min = get_minute();
    uint8_t hour = get_hour();
    uint8_t day = get_day();
    uint8_t month = get_month();
    uint16_t year = get_year();

    // Handle century wrap (assume 20xx)
    if (year < 70) year += 2000;
    else year += 1900;

    // Days in each month (non-leap year)
    const uint16_t days_before_month[] = {
        0, 31, 59, 90, 120, 151,
        181, 212, 243, 273, 304, 334
    };

    // Check leap year
    bool is_leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));

    // Total days since 1970
    uint32_t days = (year - 1970) * 365 + ((year - 1969) / 4) - ((year - 1901) / 100) + ((year - 1601) / 400);
    days += days_before_month[month - 1];
    if (is_leap && month > 2) days++;

    days += day - 1;

    return ((days * 24 + hour) * 60 + min) * 60 + sec;
}

/// @brief Turns UNIX timestamp into a string date & time
/// @param ts UNIX timestamp
/// @return String date & time (YYYY-MM-DD HH:MM:SS)
data::string rtc::timestamp_to_string(uint32_t ts) {
    data::string out;

    // Extract seconds, minutes, hours
    uint32_t seconds = ts % 60;
    ts /= 60;
    uint32_t minutes = ts % 60;
    ts /= 60;
    uint32_t hours = ts % 24;
    ts /= 24;

    // Days since 1970-01-01
    uint32_t days = ts;

    // Calculate year
    uint32_t year = 1970;
    while (true) {
        bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        uint32_t days_in_year = leap ? 366 : 365;
        if (days >= days_in_year) {
            days -= days_in_year;
            year++;
        } else {
            break;
        }
    }

    // Calculate month
    static const uint32_t days_in_month[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    uint32_t month = 0;
    bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    while (month < 12) {
        uint32_t dim = days_in_month[month];
        if (leap && month == 1) dim++; // February in leap year
        if (days >= dim) {
            days -= dim;
            month++;
        } else {
            break;
        }
    }

    uint32_t day = days + 1;

    // Helper lambda to convert number to string
    auto append_number = [&out](uint32_t num, int width) {
        char buf[5]; // Enough for 4 digits + null
        buf[width] = '\0';
        for (int i = width - 1; i >= 0; i--) {
            buf[i] = '0' + (num % 10);
            num /= 10;
        }
        out.append(buf);
    };

    // Build the string YYYY-MM-DD HH:MM:SS
    append_number(year, 4);
    out.append("-");
    append_number(month+1, 2);
    out.append("-");
    append_number(day, 2);
    out.append(" ");
    append_number(hours, 2);
    out.append(":");
    append_number(minutes, 2);
    out.append(":");
    append_number(seconds, 2);

    return out;
}

char* rtc::get_time() {
    static char time[64];
    time[0] = '\0';

    strcat(time, num_to_string(rtc::get_hour()));
    strcat(time, ":");
    strcat(time, num_to_string(rtc::get_minute()));
    strcat(time, ":");
    strcat(time, num_to_string(rtc::get_second()));

    return time;
}
