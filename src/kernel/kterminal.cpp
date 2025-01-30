// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// kterminal.cpp
// In charge of the legacy kernel level terminal
// ========================================

#include <kterminal.hpp>
#include <drivers/vga_print.hpp>

// Tells us if were on the terminal mode
bool onTerminal = false;
// Current input
char* currentInput;

// Initializes the terminal
void cmd::init(void) {
    onTerminal = true;

    // Setting up VGA enviorment for terminal
    vga::printf("\n ===========Type \"help\" to get available commands\n");
    vga::print_set_color(PRINT_COLOR_LIGHT_GREEN, PRINT_COLOR_BLACK);
    vga::printf("IoOS>$ ");
    vga::print_set_color(PRINT_COLOR_GREEN, PRINT_COLOR_BLACK);
}

// Runs a command
void cmd::run_cmd(void) {

}
