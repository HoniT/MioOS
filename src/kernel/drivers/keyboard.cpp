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
#include <interrupts/idt.hpp>
#include <lib/string_util.hpp>

using namespace kbrd;

bool shift, capsLock; // Two bool variables to manage lowercase and uppercase
// Flag to track extended scancode
static bool is_extended = false;

/// @brief Translates a raw scancode to a character / key
/// @param scancode Raw scancode gotten from inb
/// @param shift If we're pressing shift
/// @param capsLock If caps lock is on
/// @return Key base / character gotten from scancode
uint32_t get_terminal_key(uint8_t scancode, bool shift, bool capsLock) {
    if (scancode >= 128)
        return UNKNOWN;

    uint32_t base = shift ? uppercase[scancode] : lowercase[scancode];

    // Handle alphabetic caps lock behavior
    if (lowercase[scancode] < 0xF0000000 || uppercase[scancode] < 0xF0000000) {
        bool useUpper = capsLock ^ shift;
        return useUpper ? uppercase[scancode] : lowercase[scancode];
    }

    return base;
}

KeyEvent keyboardBuffer[KEYBOARD_BUFFER_SIZE];
int kb_buf_head = 0; // Where the next event will be inserted
int kb_buf_tail = 0; // Where the next event will be read from

// Adds a key event to the buffer (called by the keyboard driver / ISR)
void kbrd::push_key_event(KeyEvent ev) {
    int next = (kb_buf_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != kb_buf_tail) {  // Not full
        keyboardBuffer[kb_buf_head] = ev;
        kb_buf_head = next;
    }
}

// Removes a key event from the buffer and returns it in 'out'
// Returns true if successful, false if buffer was empty
bool kbrd::pop_key_event(KeyEvent& out) {
    if (kb_buf_head == kb_buf_tail)
        return false; // Empty

    out = keyboardBuffer[kb_buf_tail];
    kb_buf_tail = (kb_buf_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return true;
}

// Handles input
void keyboardHandler(InterruptRegisters* regs) {
    // Getting data
    uint8_t raw_scancode = io::inPortB(KBD_DATA_PORT);
    uint8_t scancode = raw_scancode & 0x7F;
    uint8_t press_state = raw_scancode & 0x80;

    if (raw_scancode == 0xE0) {
        is_extended = true;
        return;
    }

    KeyEvent event{};
    event.pressed = (press_state == 0);

    if (is_extended) {
        switch (scancode) {
            case 0x1C: event.character = '\n'; break; // Numpad Enter
            case 72:   event.character = UP;   break;
            case 80:   event.character = DOWN; break;
            default:   event.character = UNKNOWN; break;
        }
        is_extended = false;
    } else {
        switch (scancode) {
            case 0x2A: // LShift
            case 0x36: // RShift
                shift = (press_state == 0);
                return;

            case 0x3A: // Caps Lock
                if (!press_state)
                    capsLock = !capsLock;
                return;

            default:
                event.character = get_terminal_key(scancode, shift, capsLock);
                break;
        }
    }

    // Calling event
    push_key_event(event);
}

// Sets IRQ1 to the keyboard handler
void kbrd::init(void) {
    vga_coords coords = vga::set_init_text("Setting up keyboard drivers");
    // Setting to lowercase originally
    shift = false; capsLock = false;

    // Setting up IRQ handler
    idt::irq_install_handler(1, &keyboardHandler);
    vga::set_init_text_answer(coords, idt::check_irq(1, &keyboardHandler));
}
