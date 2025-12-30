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
#include <sched/process.hpp>
#include <lib/math.hpp>

void cmd::sys_cli::register_app() {
    cmd::register_command("sysinfo", sysinfo, "", " - Prints system software and hardware information");
    cmd::register_command("uptime", uptime, "", " - Prints how much time the systems been on since booting");
    cmd::register_command("currtime", currtime, "", " - Prints current time");
    cmd::register_command("lsprcss", lsprocesses, "", " - Lists active processes");
}

void cmd::sys_cli::sysinfo() {
    vga_section section = vga::create_section({vga::col_num + 15, vga::row_num}, {vga::screen_col_num, vga::row_num + 9});

    // ASCII art
    kprintf(RGB_COLOR_BLUE, " __       __ \n");
    kprintf(RGB_COLOR_BLUE, "/  \\     /  |\n");
    kprintf(RGB_COLOR_BLUE, "$$  \\   /$$ |\n");
    kprintf(RGB_COLOR_BLUE, "$$$  \\ /$$$ |\n");
    kprintf(RGB_COLOR_BLUE, "$$$$  /$$$$ |\n");
    kprintf(RGB_COLOR_BLUE, "$$ $$ $$/$$ |\n");
    kprintf(RGB_COLOR_BLUE, "$$ |$$$/ $$ |\n");
    kprintf(RGB_COLOR_BLUE, "$$ | $/  $$ |\n");
    kprintf(RGB_COLOR_BLUE, "$$/      $$/ \n");

    // RAM
    kprintf(section, "---Hardware---\n");
    kprintf(section, RGB_COLOR_LIGHT_GRAY, "RAM: %C%S\n", default_rgb_color, get_units(pmm::total_installed_ram));
    // CPU
    kprintf(section, RGB_COLOR_LIGHT_GRAY, "CPU Vendor: %C%s\n", default_rgb_color, cpu_vendor);
    kprintf(section, RGB_COLOR_LIGHT_GRAY, "CPU Model: %C%s\n", default_rgb_color, cpu_model_name);
    // VGA
    kprintf(section, RGB_COLOR_LIGHT_GRAY, "Screen resolution: %C%ux%u\n", default_rgb_color, vga::screen_width, vga::screen_height);

    // Kernel
    kprintf(section, "\n---Software---\n");
    kprintf(section, RGB_COLOR_LIGHT_GRAY, "Kernel Version: %C%s\n", default_rgb_color, kernel_version);
    kprintf(section, RGB_COLOR_LIGHT_GRAY, "Build: %C%s at %s\n", default_rgb_color, __DATE__, __TIME__);
    kprintf(section, RGB_COLOR_LIGHT_GRAY, "Compiler: %C%s", default_rgb_color, __VERSION__);

    return;
}

void cmd::sys_cli::uptime() {
    pit::getuptime();
}

void cmd::sys_cli::currtime() {
    kprintf("Date (DD/MM/YY): %u/%u/%u (%s) Time (UTC): %u:%u:%u\n", rtc::get_day(), rtc::get_month(), rtc::get_year(), weekdays[rtc::get_weekday() - 1],
    rtc::get_hour(), rtc::get_minute(), rtc::get_second());
}

void cmd::sys_cli::lsprocesses() {
    for(Process* p : process_log_list) {

        const char* state;
        switch (p->get_state())
        {
            case PROCESS_READY:
                state = "READY";
                break;
            case PROCESS_BLOCKED:
                state = "BLOCKED";
                break;
            case PROCESS_RUNNING:
                state = "RUNNING";
                break;
            case PROCESS_TERMINATED:
                state = "TERMINATED";
                break;
            default:
                break;
        }

        kprintf("PID: %u, Name: %s, Stack: %x, Priority: %u, State: %s\n", p->get_pid(), p->get_name(), p->get_stack(), p->get_priority(), 
            state);
    }
}

