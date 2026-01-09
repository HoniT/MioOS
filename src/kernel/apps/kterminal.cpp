// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// kterminal.cpp
// In charge of the legacy kernel level terminal
// ========================================

#include <apps/kterminal.hpp>
#include <graphics/vga_print.hpp>
#include <drivers/vga.hpp>
#include <drivers/keyboard.hpp>
#include <mm/heap.hpp>
#include <apps/mem_cli.hpp>
#include <apps/sys_cli.hpp>
#include <apps/storage_cli.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <x86/cpuid.hpp>
#include <drivers/pit.hpp>
#include <drivers/ata.hpp>
#include <fs/ext/ext2.hpp>
#include <fs/ext/vfs.hpp>
#include <device.hpp>
#include <kernel_main.hpp>
#include <drivers/rtc.hpp>
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
void help();
void clear();
void echo();

// List of commands
data::list<Command> commands = data::list<Command>();

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
                vga::set_cursor_updatability(false);
                cmd::save_cmd();
                cmd::run_cmd();
                vga::set_cursor_updatability(true);
                vga::update_cursor();
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

/// @brief Adds/Registers a command to kterminal command list
void cmd::register_command(const char* name,
    CommandFunc function,
    const char* params,
    const char* description) {
    if(!name || strcmp(name, ""), !function) return;

    commands.add({name, function, params, description});
}

// Initializes the terminal
void cmd::init(void) {
    currentInput = (char*)kmalloc(INPUT_MAX_SIZE);
    vfs::currentDir = "/";

    // Registering apps
    cmd::register_command("help", help, "", " - Prints available command");
    cmd::register_command("clear", clear, "", " - Clears screen");
    cmd::register_command("echo", echo, " <message>", " - Prints a message");
    cmd::mem_cli::register_app();
    cmd::storage_cli::register_app();
    cmd::sys_cli::register_app();

    // Setting up VGA enviorment for terminal
    kprintf("\n =====================Type \"help\" to get available commands==================== \n");
    kprintf(RGB_COLOR_LIGHT_GRAY, "\n%s@MioOS: %S# ", currentUser, vfs::currentDir);

    // Saving the current screen coordinates
    input_col = vga::col_num;
    input_row = vga::row_num;

    for(uint8_t i = 0; i < INPUTS_TO_SAVE; i++) 
        saved_inputs[i] = (char*)kmalloc(INPUT_MAX_SIZE);

    onTerminal = true;

    vga::set_cursor_updatability(true);
    vga::update_cursor();
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
    for(int i = 0; i < commands.count(); i++) {
        // If the current input and the command at this index match well execute
        if(strcmp(get_first_word(currentInput), commands[i].name) == 0) {
            commands[i].function();
            
            kprintf(RGB_COLOR_LIGHT_GRAY, "\n%s@MioOS: %S# ", currentUser, vfs::currentDir);
            
            // Saving the current screen coordinates
            input_col = vga::col_num;
            input_row = vga::row_num;

            // Clearing the input
            memset(currentInput, 0, INPUT_MAX_SIZE);
            return;
        }
    }

    // If we made it to here that means that the inputted command could not be found
    kprintf(LOG_INFO, "%C%s%C isn't a valid command!\n", RGB_COLOR_LIGHT_BLUE, get_first_word(currentInput), default_rgb_color);
    new_cmd:
    kprintf(RGB_COLOR_LIGHT_GRAY, "\n%s@MioOS: %S# ", currentUser, vfs::currentDir);

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

void help() {
    // Itterating through the commands
    for(int i = 0; i < commands.count(); i++) {
        kprintf(RGB_COLOR_LIGHT_BLUE, "%s", commands[i].name);
        kprintf(RGB_COLOR_BLUE, "%s", commands[i].params);
        kprintf("%s\n", commands[i].description);
    }

    return;
}


void clear() {
    vga::clear_screen();
    kprintf(" =====================Type \"help\" to get available commands==================== \n");
}

void echo() {
    kprintf("%s\n", get_remaining_string(currentInput));
}

#pragma endregion
