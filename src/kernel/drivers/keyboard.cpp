// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// keyboard.cpp
// In charge of setting up the keyboard drivers
// ========================================

#include <drivers/keyboard.hpp>
#include <io.hpp>
#include <drivers/vga_print.hpp>
#include <kterminal.hpp>
#include <interrupts/idt.hpp>
#include <lib/string_util.hpp>

using namespace keyboard;

bool capsOn, capsLock; // Two bool variables to manage lowercase and uppercase
    // Flag to track extended scancode
    static bool is_extended = false;

// Handles input
void keyboardHandler(InterruptRegisters* regs) {
    // Getting scancode and press state
    uint8_t raw_scancode = io::inPortB(KBD_DATA_PORT);
    uint8_t scancode = raw_scancode & 0x7F;  // Mask out the "pressed/released" bit
    uint8_t press_state = raw_scancode & 0x80; // Pressed down or released

    // Check for extended scancode
    if (raw_scancode == 0xE0) {
        is_extended = true;
        return; // Wait for the next byte of the extended scancode
    }

    // Handle scancode
    if (is_extended) {
        switch (scancode) {
            case 0x1C:  // Numpad Enter (extended)
                if (press_state == 0) {
                    cmd::save_cmd();
                    cmd::run_cmd(); // Same as regular Enter
                }
                break;

            case 72: // Up arrow
                if (press_state == 0) 
                    cmd::cmd_up();
                break;
            
            case 80: // Down arrow
                if (press_state == 0) 
                    cmd::cmd_down();
                break;

            default:
                break; // Ignore other extended keys for now
        }
        is_extended = false; // Reset the flag
    } else {
        switch (scancode) {
            case 0x1C:  // Regular Enter
                if (press_state == 0) {
                    cmd::save_cmd();
                    cmd::run_cmd();
                }
                break;

            case 0x2A:  // Shift key
                capsOn = (press_state == 0);
                break;

            case 0x3A:  // Caps Lock key
                if (press_state == 0) {
                    capsLock = !capsLock;
                }
                break;

            case 0x0E:  // Backspace
                if (press_state == 0 && strlen(cmd::currentInput) > 0) {
                    vga::backspace();
                    cmd::currentInput[strlen(cmd::currentInput) - 1] = '\0';
                }
                break;

            default:  // Handle printable characters
                if (press_state == 0 && cmd::onTerminal && strlen(cmd::currentInput) < 256) {
                    size_t len = strlen(cmd::currentInput);
                    if (capsOn || capsLock) {
                        vga::printf("%c", (char)(uppercase[scancode]));
                        cmd::currentInput[len] = (char)(uppercase[scancode]);
                    } else {
                        vga::printf("%c", (char)(lowercase[scancode]));
                        cmd::currentInput[len] = (char)(lowercase[scancode]);
                    }
                    cmd::currentInput[len + 1] = '\0';
                }
                break;
        }
    }
}

// Sets IRQ1 to the keyboard handler
void keyboard::init(void) {
    // Setting to lowercase originally
    capsOn = false; capsLock = false;

    // Setting up IRQ handler
    idt::irq_install_handler(1, &keyboardHandler);
    vga::printf("Initialized keyboard drivers!\n");
}
