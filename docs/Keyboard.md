# Keyboard Input Subsystem

This document outlines the architecture and usage of the MioOS keyboard driver. The driver operates on an interrupt-driven model, processing raw PS/2 scancodes into usable character events and buffering them for asynchronous retrieval.

## Architecture Overview

The keyboard driver functions as a bridge between hardware interrupts and the kernel/user applications.

1.  **Interrupt Handling:** Upon a key press or release, the keyboard triggers IRQ 1.
2.  **Scancode Processing:** The driver reads the raw scancode from the PS/2 controller (Port `0x60`).
3.  **Translation:** The scancode is translated into an ASCII character or a special keycode based on the current modifier state (Shift, Caps Lock, etc.).
4.  **Event Generation:** A `KeyEvent` structure is created containing the key data and its state (Pressed/Released).
5.  **Buffering:** The event is pushed into a **First-In-First-Out (FIFO)** buffer, allowing the system to handle bursts of input without losing keystrokes.

---

## Data Structures

### KeyEvent
The core unit of input in MioOS is the `KeyEvent`. It encapsulates the result of a single physical key action.

| Field | Type | Description |
| :--- | :--- | :--- |
| **Character** | `char` | The ASCII representation of the key (e.g., 'a', 'A', '1'). |
| **State** | `uint8_t` | The action type:<br>`0`: **Key Down** (Pressed)<br>`1`: **Key Up** (Released) |

---

## Driver API Reference

The driver exposes methods for the Interrupt Service Routine (ISR) to push data and for the kernel/applications to pop data.

| Function | Signature | Description |
| :--- | :--- | :--- |
| **Push Event** | `kbrd::push_key_event(KeyEvent ev)` | **Internal/ISR Use.** Adds a new `KeyEvent` to the end of the input buffer. If the buffer is full, the event may be discarded or overwrite old data (depending on implementation). |
| **Pop Event** | `kbrd::pop_key_event(KeyEvent& out)` | **Consumer Use.** Retrieves the next pending event from the front of the buffer.<br>**Returns:** `true` if an event was retrieved, `false` if the buffer was empty. |

---

## Usage Example

To process input, the kernel (or a terminal task) should poll the buffer or sleep until an interrupt signals new data. Below is a standard polling loop pattern, similar to the implementation in `kterminal_handle_input()`:

```cpp
void handle_input() {
    KeyEvent event;

    // Process all pending events in the buffer
    while (kbrd::pop_key_event(event)) {
        
        // Only act on Key Down (0)
        if (event.state == 0) {
            if (event.character != 0) {
                // It's a printable character
                kprintf("%c", event.character);
            } else {
                // It's a special key (e.g., Ctrl, Alt)
                handle_special_key(event);
            }
        }
    }
}