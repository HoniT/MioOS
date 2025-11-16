// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// kterminal.cpp
// In charge of the legacy kernel level terminal
// ========================================

#include <kterminal.hpp>
#include <graphics/vga_print.hpp>
#include <drivers/vga.hpp>
#include <drivers/keyboard.hpp>
#include <mm/heap.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <cpuid.hpp>
#include <pit.hpp>
#include <drivers/ata.hpp>
#include <fs/ext/ext2.hpp>
#include <fs/ext/vfs.hpp>
#include <device.hpp>
#include <kernel_main.hpp>
#include <rtc.hpp>
#include <lib/string_util.hpp>
#include <lib/math.hpp>

using namespace kbrd;

// Tells us if were on the terminal mode
bool cmd::onTerminal = false;
// Current input
char* currentInput;
data::string cmd::get_input() {return data::string(currentInput);}

// Getter
const char* get_current_input() {
    return currentInput;
}

// Basic command function declarations
void help(data::list<data::string> params);
void clear(data::list<data::string> params);
void echo(data::list<data::string> params);
// System info commands
void getsysinfo(data::list<data::string> params);
// Memory debugging commands
void peek(data::list<data::string> params);
void poke(data::list<data::string> params);

// List of commands
Command commands[] = {
    {"help", help, "", " - Prints available commands"},
    {"clear", clear, "", " - Clears the screen"},
    {"echo", echo, " <message>", " - Writes a given message back to the terminal"},
    // For system information
    {"getsysinfo", getsysinfo, "", " - Prints system software and hardware information"},
    {"getmeminfo", pmm::getmeminfo, "", " - Prints system memory info"},
    {"getuptime", pit::getuptime, "", " - Prints how much time the systems been on since booting"},
    {"gettime", rtc::print_time, "", " - Prints current time"},
    // Memory debugging commands
    {"peek", peek, " <address>", " - Prints a value at a given physical address"},
    {"poke", poke, " <address> <value>", " - Writes to a given address a given value"},
    {"heapdump", heap::heap_dump, "", " - Prints the allocation status of blocks in the heap"},
    // Storage commands
    {"read_ata", ata::read_ata, " -dev <device_index> -sect <sector_index>", " - Prints a given sector of a given ATA device"},
    {"list_ata", ata::list_ata, "", " - Lists available ATA devices"},
    {"pwd", ext2::pwd, "", " - Prints working directory"},
    {"ls", ext2::ls, "", " - Lists entries of the current directory"},
    {"cd", ext2::cd, " <dir>", " - Changes directory to given dir"},
    {"mkdir", ext2::mkdir, " <dir>", " - Creates a directory in the current dir"},
    {"mkfile", ext2::mkfile, " <file>", " - Creates a file in the current dir"},
    {"rm", ext2::rm, " <file>", " - Removes (deletes) a directory/directory entry"},
    {"cat", ext2::cat, " <file>", " - Prints file contents"},
    {"write", ext2::write_to_file, " <file> <content>", " - Writes something to a file"},
    {"append", ext2::write_to_file_append, " <file> <content>", " - Append something to a file"}
};

// Saving inputs
char* saved_inputs[INPUTS_TO_SAVE]; // This is the saved inputs
uint8_t input_write_index = 0; // This is the index where the current input is being written in the saved inputs list
uint8_t input_read_index = 0; // This is the index of witch saved input will be read and displayed
uint8_t saved_inputs_num = 0; // This counts how many places are occupied in the saved_inputs array

// The coordinates of the screen in which the current input resides
size_t input_row, input_col;

char* cmd::currentUser = "root"; // Current user using the terminal

// Handles keyboard input
void kterminal_handle_input() {
    KeyEvent ev;
    while (kbrd::pop_key_event(ev)) {
        if (!ev.pressed)
            continue; // Ignore key releases

        switch (ev.character) {
            case '\n':
                cmd::save_cmd();
                cmd::run_cmd();
                break;

            case '\b':
                if (strlen(currentInput) > 0) {
                    vga::backspace();
                    currentInput[strlen(currentInput) - 1] = '\0';
                }
                break;

            case UP:
                cmd::cmd_up();
                break;

            case DOWN:
                cmd::cmd_down();
                break;

            case UNKNOWN:
                break;

            default:
                if (cmd::onTerminal && strlen(currentInput) < 256) {
                    size_t len = strlen(currentInput);
                    currentInput[len] = (char)ev.character;
                    currentInput[len + 1] = '\0';
                    kprintf("%c", ev.character);
                }
                break;
        }
    }
}


// Initializes the terminal
void cmd::init(void) {
    currentInput = (char*)kmalloc(INPUT_MAX_SIZE);
    vfs::currentDir = "/";

    // Setting up VGA enviorment for terminal
    kprintf("\n =====================Type \"help\" to get available commands==================== \n");
    kprintf(RGB_COLOR_LIGHT_GRAY, "%s@MioOS: %S# ", currentUser, vfs::currentDir);

    // Saving the current screen coordinates
    input_col = vga::col_num;
    input_row = vga::row_num;

    for(uint8_t i = 0; i < INPUTS_TO_SAVE; i++) 
        saved_inputs[i] = (char*)kmalloc(INPUT_MAX_SIZE);

    onTerminal = true;

    while (true) {
        kterminal_handle_input();
    }
}

// Runs a command
void cmd::run_cmd(void) {
    kprintf("\n");
    if(strcmp(get_first_word(currentInput), "") == 0) {
        goto new_cmd;
    }

    // Reseting the read index
    input_read_index = saved_inputs_num;

    // Itterating through the commands
    for(int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
        // If the current input and the command at this index match well execute
        if(strcmp(get_first_word(currentInput), commands[i].name) == 0) {
            data::list<data::string> params = split_string_tokens(get_remaining_string(get_current_input())); 
            commands[i].function(params);
            params.~list();
            
            kprintf(RGB_COLOR_LIGHT_GRAY, "%s@MioOS: %S# ", currentUser, vfs::currentDir);
            
            // Saving the current screen coordinates
            input_col = vga::col_num;
            input_row = vga::row_num;

            // Clearing the input
            memset(currentInput, 0, INPUT_MAX_SIZE);
            return;
        }
    }

    // If we made it to here that means that the inputted command could not be found
    kprintf(LOG_WARNING, "%s isn't a valid command!\n", get_first_word(currentInput));
    new_cmd:
    kprintf(RGB_COLOR_LIGHT_GRAY, "%s@MioOS: %S# ", currentUser, vfs::currentDir);

    // Saving the current screen coordinates
    input_col = vga::col_num;
    input_row = vga::row_num;

    // Clearing the input
    memset(currentInput, 0, INPUT_MAX_SIZE);
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
        vga::clear_text_region(input_col, input_row, strlen(currentInput));
        // Setting the selected command as current
        strcpy(currentInput, saved_inputs[--input_read_index]);
        // Setting up new text
        vga::insert(input_col, input_row, true, currentInput);
    }
}

// Goes down in the saved commands/inputs list
void cmd::cmd_down(void) {
    /* If we're in the bounds of the array, we'll increment the read index and 
     * copy the corresponding saved input to the current input */
    if(input_read_index < saved_inputs_num) {
        // Clearing up the old text
        vga::clear_text_region(input_col, input_row, strlen(currentInput));
        // Setting the selected command as current
        strcpy(currentInput, saved_inputs[++input_read_index]);
        // Setting up new text
        vga::insert(input_col, input_row, true, currentInput);
    }
    // If we'll go out of the bounds, we will make the current input empty
    else if(input_read_index == saved_inputs_num) {
        // Clearing up the old text
        vga::clear_text_region(input_col, input_row, strlen(currentInput));
        // Setting the selected command as current
        strcpy(currentInput, "");
        // Setting up new text
        vga::insert(input_col, input_row, true, currentInput);
    }
}


// ====================
// Commands
// ====================

#pragma region Basic Commands

void help(data::list<data::string> params) {
    // Itterating through the commands
    for(int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
        kprintf(RGB_COLOR_LIGHT_BLUE, "%s", commands[i].name);
        kprintf(RGB_COLOR_BLUE, "%s", commands[i].params);
        kprintf("%s\n", commands[i].description);
    }

    return;
}

void getsysinfo(data::list<data::string> params) {
    // RAM
    kprintf("---Hardware---\nRAM: %lu GiB\n", (pmm::total_installed_ram / BYTES_IN_GIB));

    // CPU
    kprintf("CPU Vendor: %s\n", cpu_vendor);
    kprintf("CPU Model: %s\n", cpu_model_name);

    // Kernel
    kprintf("\n---Software---\nKernel Version: %S\n", kernel_version);

    return;
}

void clear(data::list<data::string> params) {
    vga::clear_screen();
    kprintf(" =====================Type \"help\" to get available commands==================== \n");
}

void echo(data::list<data::string> params) {
    kprintf("%s\n", get_remaining_string(currentInput));
}

void peek(data::list<data::string> params) {
    // Getting the address from the input
    const char* strAddress = params.at(0);
    uint32_t address = hex_to_uint32(strAddress);
    #ifdef VMM_HPP
    if(!vmm::is_mapped(address)) {
        kprintf(LOG_ERROR, "The given address %x is not mapped in virtual memory!\n", address);
        return;
    }
    #endif
    // Printing the value
    kprintf("Value at the given address: %x\n", *(uint32_t*)address);
}

void poke(data::list<data::string> params) {
    if(params.count() != 2) {
        kprintf(LOG_WARNING, "Syntax: poke <address> <value>\n");
        return;
    }

    // Getting address and value from the input
    const char* strAddress = params.at(0);
    uint32_t address = hex_to_uint32(strAddress);
    #ifdef VMM_HPP
    if(!vmm::is_mapped(address)) {
        kprintf(LOG_ERROR, "The given address %x is not mapped in virtual memory!\n", address);
        return;
    }
    #endif
    const char* strVal = params.at(1);
    uint8_t val = hex_to_uint32(strVal);

    // Writing the value to the address
    *(uint8_t*)address = val;
}

#pragma endregion
