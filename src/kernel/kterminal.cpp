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
#include <drivers/ata.hpp>
#include <device.hpp>
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
// Storage commands
void read_ata();
void list_ata();

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
    {"heapdump", heap::heap_dump, " - Prints the allocation status of blocks in the heap"},
    // Storage commands
    {"read_ata", read_ata, " -dev <device_index> -sect <sector_index> - Prints a given sector of a given ATA device"},
    {"list_ata", list_ata, " - Lists available ATA devices"}
};

// Saving inputs
char* saved_inputs[INPUTS_TO_SAVE]; // This is the saved inputs
uint8_t input_write_index = 0; // This is the index where the current input is being written in the saved inputs list
uint8_t input_read_index = 0; // This is the index of witch saved input will be read and displayed
uint8_t saved_inputs_num = 0; // This counts how many places are occupied in the saved_inputs array

// The coordinates of the screen in which the current input resides
size_t input_row, input_col;

char* currentDir = "~"; // Current directory in fs to display in terminal
char* currentUser = "root"; // Current user using the terminal

// Initializes the terminal
void cmd::init(void) {
    onTerminal = true;
    currentInput = (char*)kmalloc(255);

    // Setting up VGA enviorment for terminal
    vga::printf("\n ===========Type \"help\" to get available commands\n");
    vga::print_set_color(PRINT_COLOR_LIGHT_GRAY, PRINT_COLOR_BLACK);
    vga::printf("%s@MioOS: %s# ", currentUser, currentDir);
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
            vga::printf("%s@MioOS: %s# ", currentUser, currentDir);
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
    vga::warning("\n%s isn't a valid command!\n", get_first_word(currentInput));
    vga::print_set_color(PRINT_COLOR_LIGHT_GRAY, PRINT_COLOR_BLACK);
    vga::printf("%s@MioOS: %s# ", currentUser, currentDir);
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
    vga::printf("---Hardware---\nRAM: %lu GiB\n", (pmm::total_installed_ram / BYTES_IN_GIB));

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
        vga::printf("Total installed memory:  %lu bytes (~%lu GiB)\n", pmm::total_installed_ram, pmm::total_installed_ram / BYTES_IN_GIB);
        vga::printf("Available usable memory: %lu bytes (~%lu GiB)\n", pmm::total_usable_ram, pmm::total_usable_ram / BYTES_IN_GIB);
        vga::printf("Used memory:                %lu bytes\n", pmm::total_used_ram);
        vga::printf("Hardware reserved memory:   %lu bytes\n", pmm::hardware_reserved_ram);
        return;
    }
    
    // If an invalid flag has been entered we'll throw an error
    vga::warning("Invalid flag \"%s\"for \"getmeminfo\"!\n", get_remaining_string(currentInput));
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
    uint32_t address = hex_to_uint32(strAddress);
    #ifdef VMM_HPP
    if(!vmm::is_mapped(address)) {
        vga::error("The given address %x is not mapped in virtual memory!\n", address);
        return;
    }
    #endif
    // Printing the value
    vga::printf("Value at the given address: %h\n", *(uint8_t*)address);
}

void poke() {
    // Getting address and value from the input
    const char* strAddress = get_first_word(get_remaining_string(currentInput));
    uint32_t address = hex_to_uint32(strAddress);
    #ifdef VMM_HPP
    if(!vmm::is_mapped(address)) {
        vga::error("The given address %x is not mapped in virtual memory!\n", address);
        return;
    }
    #endif
    const char* strVal = get_remaining_string(get_remaining_string(currentInput));
    uint8_t val = hex_to_uint32(strVal);

    // Writing the value to the address
    *(uint8_t*)address = val;
}

// Reads a given sector on a given ATA device 
void read_ata() {
    // Getting arguments from string
    const char* args = get_remaining_string(currentInput);
    if(strlen(args) == 0 || get_words(args) != 4 || strcmp(get_word_at_index(args, 0), "-dev") != 0 || strcmp(get_word_at_index(args, 2), "-sect") != 0) {
        vga::warning("Syntax: read_mbr -dev <device_index> -sect <sector_index>\n");
        return;
    }

    int device_index = str_to_int(get_word_at_index(args, 1));
    if(device_index < 0 || device_index >= 4) {
        vga::warning("Please use an integer (0-3) as the device index in decimal format.\n");
        return;
    }

    int sector_index = str_to_int(get_word_at_index(args, 3));
    if(sector_index < 0 || sector_index >= ata_devices[device_index].total_sectors) {
        vga::warning("Please use a decimal integer as the sector index. Make sure it's in the given devices maximum sector count: %u\n", ata_devices[device_index].total_sectors);
        return;
    }

    uint16_t buffer[256];
    if(pio_28::read_sector(&(ata_devices[device_index]), sector_index, buffer))
        for(uint16_t i = 0; i < 256; i++) 
            vga::printf("%h ", buffer[i]);
}

void list_ata(void) {
    for(int i = 0; i < 4; i++) {
        if(strlen(ata_devices[i].serial) == 0) continue;
        ata::device_t* device = &(ata_devices[i]); 
        vga::printf("\nModel: %s, serial: %s, firmware: %s, total sectors: %u, lba_support: %h, dma_support: %h ", 
            device->model, device->serial, device->firmware, device->total_sectors, (uint32_t)device->lba_support, (uint32_t)device->dma_support);
        vga::printf("IO information: bus: %s, drive: %s\n", device->bus == ata::Bus::Primary ? "Primary" : "Secondary", device->drive == ata::Drive::Master ? "Master" : "Slave");
    }
}

#pragma endregion
