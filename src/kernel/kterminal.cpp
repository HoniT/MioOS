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
#include <mm/vmm.hpp>
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
void print_inputs();
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
    {"printinputs", print_inputs, " - Writes the list of old inputs"},
    // For system information
    {"getsysinfo", getsysinfo, " - Prints system software and hardware information"},
    {"getmeminfo", getmeminfo, " - Prints system memory info"},
    {"getuptime", getuptime, " - Prints how much time the systems been on since booting"},
    // Memory debugging commands
    {"peek", peek, " <address> - Prints a value at a given physical address"},
    {"poke", poke, " <address> <value> - Writes to a given address a given value"},
    {"heapdump", heap::heap_dump, " - Prints the allocation status of blocks in the heap"}
};

// Saving inputs
char* saved_inputs[INPUTS_TO_SAVE]; // This is the saved inputs
uint8_t input_write_index = 0; // This is the index where the current input is being written in the saved inputs list
uint8_t input_read_index = 0; // This is the index of witch saved input will be read and displayed
uint8_t saved_inputs_num = 0; // This counts how many places are occupied in the saved_inputs array

// The coordinates of the screen in which the current input resides
size_t input_row, input_col;

// Initializes the terminal
void cmd::init(void) {
    onTerminal = true;
    currentInput = (char*)kmalloc(255);

    // Setting up VGA enviorment for terminal
    vga::printf("\n ===========Type \"help\" to get available commands\n");
    vga::print_set_color(PRINT_COLOR_LIGHT_GRAY, PRINT_COLOR_BLACK);
    vga::printf("#MioOS#KERNEL#>$ ");
    vga::print_set_color(PRINT_COLOR_GREEN, PRINT_COLOR_BLACK);

    // Saving the current screen coordinates
    input_col = col;
    input_row = row;

    for(uint8_t i = 0; i < INPUTS_TO_SAVE; i++) 
        saved_inputs[i] = (char*)kmalloc(255);
}

// Runs a command
void cmd::run_cmd(void) {
    // Reseting the read index
    input_read_index = saved_inputs_num;

    // Itterating through the commands
    for(int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
        // If the current input and the command at this index match well execute
        if(strcmp(get_first_word(currentInput), commands[i].name) == 0) {
            vga::printf("\n");
            commands[i].function();
            
            vga::print_set_color(PRINT_COLOR_LIGHT_GRAY, PRINT_COLOR_BLACK);
            vga::printf("#MioOS#KERNEL#>$ ");
            vga::print_set_color(PRINT_COLOR_GREEN, PRINT_COLOR_BLACK);
            
            // Saving the current screen coordinates
            input_col = col;
            input_row = row;

            // Clearing the input
            kfree(currentInput);
            currentInput = (char*)kmalloc(255);
            return;
        }
    }

    // If we made it to here that means that the inputted command could not be found
    vga::printf("\n%s isn't a valid command!\n", get_first_word(currentInput));
    vga::print_set_color(PRINT_COLOR_LIGHT_GRAY, PRINT_COLOR_BLACK);
    vga::printf("#MioOS#KERNEL#>$ ");
    vga::print_set_color(PRINT_COLOR_GREEN, PRINT_COLOR_BLACK);

    // Saving the current screen coordinates
    input_col = col;
    input_row = row;

    // Clearing the input
    kfree(currentInput);
    currentInput = (char*)kmalloc(255);
    return;
}

// Saves a command
void cmd::save_cmd(void) {
    // If the current input is not empty
    if(strcmp(get_first_word(currentInput), "") != 0) {
        // If the last saved command was the same as the current one then we won't save it
        if(input_write_index == 0 || (input_write_index > 0 && strcmp(currentInput, saved_inputs[input_write_index - 1]) != 0)) {

            // If the saved inputs list is not filled
            if(input_write_index < INPUTS_TO_SAVE - 1) {
                // Saving the currentInput and incrementing the index
                strcpy(saved_inputs[input_write_index++], currentInput);
                // Noting that one more place got occupied in the saved_inputs array
                saved_inputs_num++;
            }
            // If it is filled
            else {
                // Moving the saved commands up
                for(uint8_t i = 0; i < INPUTS_TO_SAVE - 1; i++) {
                    strcpy(saved_inputs[i], saved_inputs[i + 1]);
                }
                // Wrtting the current input in the last spot
                strcpy(saved_inputs[input_write_index], currentInput);
            }
        }
    }
}

// Goes up in the saved commands/inputs list
void cmd::cmd_up(void) {
    /* If we're in the bounds of the array, we'll decrement the read index and 
     * copy the corresponding saved input to the current input */
    if(input_read_index - 1 >= 0) {
        // Clearing up the old text
        vga::clear_region(input_row, input_col, strlen(currentInput));
        // Setting the selected command as current
        strcpy(currentInput, saved_inputs[--input_read_index]);
        // Setting up new text
        vga::insert(input_row, input_col, currentInput, true);
    }
}

// Goes down in the saved commands/inputs list
void cmd::cmd_down(void) {
    /* If we're in the bounds of the array, we'll increment the read index and 
     * copy the corresponding saved input to the current input */
    if(input_read_index < saved_inputs_num) {
        // Clearing up the old text
        vga::clear_region(input_row, input_col, strlen(currentInput));
        // Setting the selected command as current
        strcpy(currentInput, saved_inputs[++input_read_index]);
        // Setting up new text
        vga::insert(input_row, input_col, currentInput, true);
    }
    // If we'll go out of the bounds, we will make the current input empty
    else if(input_read_index == saved_inputs_num) {
        // Clearing up the old text
        vga::clear_region(input_row, input_col, strlen(currentInput));
        // Setting the selected command as current
        strcpy(currentInput, "");
        // Setting up new text
        vga::insert(input_row, input_col, currentInput, true);
    }
}

void print_inputs(void) {
    // Itterating through the saved inputs
    for(uint8_t i = 0; i < INPUTS_TO_SAVE; i++) {
        // Only printing if theres something written
        if(saved_inputs[i] && strcmp(saved_inputs[i], "") != 0) {
            vga::printf("%u: %s\n", i + 1, saved_inputs[i]);
        }
    }
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

    // Prints the memory map
    if(strcmp(get_remaining_string(currentInput), "-mmap") == 0) {
        pmm::print_memory_map();
        return;
    }

    // Prints block info
    if(strcmp(get_remaining_string(currentInput), "-reg") == 0) {
        pmm::print_usable_regions();
        return;
    }

    // If there were no flags inputed
    if(strcmp(get_remaining_string(currentInput), "") == 0) {
        // Printing usable and used memory
        vga::printf("Use flag \"-h\" to get evry specific version of getmeminfo.\n");
        vga::printf("Available usable memory: ~%lu GiB\n", pmm::total_usable_ram / BYTES_IN_GIB);
        vga::printf("Used memory: %lu bytes\n", pmm::total_used_ram);
        vga::printf("Hardware reserved memory: %lu bytes\n\n", pmm::hardware_reserved_ram);
        return;
    }
    
    // If an invalid flag has been entered we'll throw an error
    vga::error("Invalid flag \"%s\"for \"getmeminfo\"!\n", get_remaining_string(currentInput));
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
    uint64_t address = hex_to_uint64(strAddress);
    if(!vmm::is_mapped(address)) {
        vga::error("The given address %lx is not mapped in virtual memory!\n", address);
        return;
    }

    // Printing the value
    vga::printf("Value at the given address: %x\n", *(uint32_t*)address);
}

void poke() {
    // Getting address and value from the input
    const char* strAddress = get_first_word(get_remaining_string(currentInput));
    uint64_t address = hex_to_uint64(strAddress);

    if(!vmm::is_mapped(address)) {
        vga::error("The given address %lx is not mapped in virtual memory!\n", address);
        return;
    }

    const char* strVal = get_remaining_string(get_remaining_string(currentInput));
    uint32_t val = hex_to_uint32(strVal);

    // Writing the value to the address
    *(uint32_t*)address = val;
}


#pragma endregion
