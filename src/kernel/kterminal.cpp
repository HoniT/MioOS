// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// kterminal.cpp
// In charge of the legacy kernel level terminal
// ========================================

#include <kterminal.hpp>
#include <drivers/vga_print.hpp>
#include <mm/heap.hpp>
#include <mm/pmm.hpp>
#include <cpuid.hpp>
#include <pit.hpp>
#include <kernel_main.hpp>
#include <lib/string_util.hpp>
#include <lib/math.hpp>

// Tells us if were on the terminal mode
bool onTerminal = false;
// Current input
char* currentInput;

// Command function declarations
void help();
void clear();
void echo();
// System info commands
void getsysinfo();
void getmeminfo();
void getuptime();
// Memory debugging commands
void peek();
void poke();

// List of commands
Command commands[] = {
    {"help", help, " - Prints available commands"},
    {"clear", clear, " - Clears the screen"},
    {"echo", echo, " <message> - Writes a given message back to the terminal"},
    // For system information
    {"getsysinfo", getsysinfo, " - Prints system software and hardware information"},
    {"getmeminfo", getmeminfo, " - Prints system memory info"},
    {"getuptime", getuptime, " - Prints how much time the systems been on since booting"},
    // Memory debugging commands
    {"peek", peek, " <address> - Prints a value at a given physical address"},
    {"poke", poke, " <address> <value> - Writes to a given address a given value"},
    {"heapdump", heap::heap_dump, " - Prints the allocation status of blocks in the heap"}
};

// Initializes the terminal
void cmd::init(void) {
    onTerminal = true;
    currentInput = (char*)(kmalloc(sizeof(char) * 256));

    // Setting up VGA enviorment for terminal
    vga::printf("\n ===========Type \"help\" to get available commands\n");
    vga::print_set_color(PRINT_COLOR_LIGHT_GRAY, PRINT_COLOR_BLACK);
    vga::printf("#MioOS#KERNEL#>$ ");
    vga::print_set_color(PRINT_COLOR_GREEN, PRINT_COLOR_BLACK);
}

// Runs a command

void cmd::run_cmd() {
    // Itterating through the commands
    for(int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
        // If the current input and the command at this index match well execute
        if(strcmp(get_first_word(currentInput), commands[i].name) == 0) {
            vga::printf("\n");
            commands[i].function();

            vga::print_set_color(PRINT_COLOR_LIGHT_GRAY, PRINT_COLOR_BLACK);
            vga::printf("#MioOS#KERNEL#>$ ");
            vga::print_set_color(PRINT_COLOR_GREEN, PRINT_COLOR_BLACK);

            // Clearing the input
            kfree(currentInput);
            currentInput = (char*)(kmalloc(sizeof(char) * 256));
            return;
        }
    }

    // If we made it to here that means that the inputted command could not be found
    vga::printf("\n%s isn't a valid command!\n", get_first_word(currentInput));
    vga::print_set_color(PRINT_COLOR_LIGHT_GRAY, PRINT_COLOR_BLACK);
    vga::printf("#MioOS#KERNEL#>$ ");
    vga::print_set_color(PRINT_COLOR_GREEN, PRINT_COLOR_BLACK);

    // Clearing the input
    kfree(currentInput);
    currentInput = (char*)(kmalloc(sizeof(char) * 256));
    return;
}

// ====================
// Commands
// ====================

#pragma region Commands

void help() {
    // Itterating through the commands
    for(int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
        vga::printf("%s%s\n", commands[i].name, commands[i].description);
    }

    return;
}

void getsysinfo() {
    // RAM
    vga::printf("---Hardware---\nRAM: %lu GiB\n", (pmm::total_usable_ram / BYTES_IN_GIB));

    // CPU
    vga::printf("CPU Vendor: %s\n", cpu_vendor);
    vga::printf("CPU Model: %s\n", cpu_model_name);

    // Kernel
    vga::printf("\n---Software---\nKernel Version: %u\n", kernel_version);

    return;
}

void clear() {
    col = 0; row = 1;
    vga::print_clear();
    vga::printf(" =====================Type \"help\" to get available commands==================== ");
}

void echo() {
    vga::printf("%s\n", get_remaining_string(currentInput));
}

void getmeminfo() {
    // If the user inputed the help flag
    if(strcmp(get_remaining_string(currentInput), "-h") == 0) {
        // Printing every available version of getmeminfo
        vga::printf("-mmap - Prints the memory map given from GRUB\n");
        vga::printf("-reg - Prints the blocks in the usable memory regions\n");
        return;
    }

    // If there were no flags inputed
    if(strcmp(get_remaining_string(currentInput), "") == 0) {
        // Printing usable and used memory
        vga::printf("Use flag \"-h\" to get evry specific version of getmeminfo.\n");
        vga::printf("Available usable memory: ~%lu GiB\n", (pmm::total_usable_ram / BYTES_IN_GIB));
        vga::printf("Used memory: %lu bytes\n\n", (pmm::total_used_ram));
        return;
    }
    
    // Prints the memory map
    if(strcmp(get_remaining_string(currentInput), "-mmap") == 0)
        pmm::print_memory_map();
    // Prints block info
    if(strcmp(get_remaining_string(currentInput), "-reg") == 0)
        pmm::print_usable_regions();
}

void getuptime() {
    uint64_t total_seconds = udiv64(ticks, uint64_t(frequency));

    uint64_t hours = udiv64(total_seconds, 3600);
    uint64_t minutes = udiv64(umod64(total_seconds, 3600), 60);
    uint64_t seconds = umod64(total_seconds, 60);

    vga::printf("Hours: %lu\n", hours);
    vga::printf("Minutes: %lu\n", minutes);
    vga::printf("Seconds: %lu\n", seconds);
}

void peek() {
    // Getting the address from the input
    const char* strAddress = get_remaining_string(currentInput);
    uint32_t* address = (uint32_t*)hex_to_uint32(strAddress);

    // Printing the value
    vga::printf("Value at the given address: %x\n", *address);
}

void poke() {
    // Getting address and value from the input
    const char* strAddress = get_first_word(get_remaining_string(currentInput));
    uint32_t* address = (uint32_t*)hex_to_uint32(strAddress);

    const char* strVal = get_remaining_string(get_remaining_string(currentInput));
    uint32_t val = hex_to_uint32(strVal);

    // Writing the value to the address
    *address = val;
}


#pragma endregion
