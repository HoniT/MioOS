// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// sys_cli.cpp
// System info CLI commands
// ========================================

#include <apps/sys_cli.hpp>
#include <apps/kterminal.hpp>
#include <mm/pmm.hpp>
#include <x86/cpuid.hpp>
#include <kernel_main.hpp>
#include <drivers/pit.hpp>
#include <drivers/rtc.hpp>
#include <drivers/vga.hpp>
#include <lib/math.hpp>

void cmd::sys_cli::register_app() {
    cmd::register_command("sysinfo", sysinfo, "", " - Prints system software and hardware information");
    cmd::register_command("uptime", uptime, "", " - Prints how much time the systems been on since booting");
    cmd::register_command("currtime", currtime, "", " - Prints current time");
}

void cmd::sys_cli::sysinfo() {
    // RAM
    kprintf("---Hardware---\n");
    kprintf("RAM:               %S\n", get_units(pmm::total_installed_ram));
    // CPU
    kprintf("CPU Vendor:        %s\n", cpu_vendor);
    kprintf("CPU Model:         %s\n", cpu_model_name);
    // VGA
    kprintf("Screen resolution: %ux%u\n", vga::screen_height, vga::screen_width);

    // Kernel
    kprintf("\n---Software---\n");
    kprintf("Kernel Version:    %s\n", kernel_version);
    kprintf("Build:             %s at %s\n", __DATE__, __TIME__);
    kprintf("Compiler:          %s\n", __VERSION__);

    return;
}

void cmd::sys_cli::uptime() {
    pit::getuptime();
}

void cmd::sys_cli::currtime() {
    kprintf("Date (DD/MM/YY): %u/%u/%u (%s) Time (UTC): %u:%u:%u\n", rtc::get_day(), rtc::get_month(), rtc::get_year(), weekdays[rtc::get_weekday() - 1],
    rtc::get_hour(), rtc::get_minute(), rtc::get_second());
}
