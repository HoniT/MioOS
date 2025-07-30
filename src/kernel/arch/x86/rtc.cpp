// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// rtc.cpp
// Sets up the Real Time Clock
// ========================================

#include <rtc.hpp>
#include <io.hpp>
#include <drivers/vga_print.hpp>

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

char* weekdays[7] = {
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"
};

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

// Prints current time
void rtc::print_time(void) {
    vga::printf("Date (DD/MM/YY): %u/%u/%u (%s) Time (UTC): %u:%u:%u\n", rtc::get_day(), rtc::get_month(), rtc::get_year(), weekdays[rtc::get_weekday() - 1], 
    rtc::get_hour(), rtc::get_minute(), rtc::get_second());
}
