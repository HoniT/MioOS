# VGA Printing Library Documentation

This library provides a high-level interface for printing text to the VGA buffer (memory address `0xB8000`) in protected mode kernel development. It supports formatted strings (printf-style), color management, semantic logging levels, and screen sections (bounding boxes).

## Integration

To use the VGA printing features, include the header file `graphics/vga_print.hpp` in your kernel source.

---

## Constants & Definitions

### VGA Text Mode Constraints

| Constant | Value | Description |
| :--- | :--- | :--- |
| `VGA_ADDRESS` | `0xB8000` | The physical memory address for the VGA buffer. |
| `NUM_COLS` | `80` | Screen width in characters. |
| `NUM_ROWS` | `25` | Screen height in characters. |
| `VGAT_COLOR` | `15` | Default white on black (`0x0F`). |

### Color Macros
Colors are defined as `uint32_t` hex values, mapping to the standard 16-color VGA palette.

| Macro | Hex Value | Color Name |
| :--- | :--- | :--- |
| `RGB_COLOR_BLACK` | `0x000000` | Black |
| `RGB_COLOR_BLUE` | `0x0000AA` | Blue |
| `RGB_COLOR_GREEN` | `0x00AA00` | Green |
| `RGB_COLOR_CYAN` | `0x00AAAA` | Cyan |
| `RGB_COLOR_RED` | `0xAA0000` | Red |
| `RGB_COLOR_MAGENTA` | `0xAA00AA` | Magenta |
| `RGB_COLOR_BROWN` | `0xAA5500` | Brown |
| `RGB_COLOR_LIGHT_GRAY` | `0xAAAAAA` | Light Gray |
| `RGB_COLOR_DARK_GRAY` | `0x555555` | Dark Gray |
| `RGB_COLOR_LIGHT_BLUE` | `0x5555FF` | Light Blue |
| `RGB_COLOR_LIGHT_GREEN` | `0x55FF55` | Light Green |
| `RGB_COLOR_LIGHT_CYAN` | `0x55FFFF` | Light Cyan |
| `RGB_COLOR_LIGHT_RED` | `0xFF5555` | Light Red |
| `RGB_COLOR_PINK` | `0xFF55FF` | Pink |
| `RGB_COLOR_YELLOW` | `0xFFFF55` | Yellow |
| `RGB_COLOR_WHITE` | `0xFFFFFF` | White |

---

## Data Structures

### PrintTypes (Enum)
Used to categorize messages. This allows the implementation to automatically assign prefixes (e.g., `[INFO]`) or specific colors to messages.

* `STD_PRINT`: Standard output.
* `LOG_INFO`: Informational logging.
* `LOG_WARNING`: Warnings (non-fatal).
* `LOG_ERROR`: Critical errors.

### vga_coords (Struct)
Represents a specific character coordinate on the screen.

* `col`: Column position (x).
* `row`: Row position (y).

### vga_section (Struct)
Defines a rectangular "window" or region on the screen. Printing to a section ensures text wraps within its boundaries and does not overwrite the rest of the screen.

* `startX`, `startY`: Top-left boundary coordinates.
* `endX`, `endY`: Bottom-right boundary coordinates.
* `col`, `row`: Current internal cursor position relative to the section.

---

## Global Printing API (kprintf)
These functions print to the main screen context. All return `vga_coords` indicating the final cursor position.

| Function Signature | Description |
| :--- | :--- |
| `kprintf(fmt, ...)` | Prints formatted text using the default system color. |
| `kprintf(color, fmt, ...)` | Prints formatted text using a specific color. |
| `kprintf(type, fmt, ...)` | Prints text styled according to the log level (Info, Warning, Error). |
| `kprintf(type, color, fmt, ...)` | Prints text with a log level prefix but forces a specific color. |

## Section Printing API
These functions print inside a `vga_section`. The text will wrap relative to the section's width.

| Function Signature | Description |
| :--- | :--- |
| `kprintf(sect, fmt, ...)` | Basic print into a specific section. |
| `kprintf(sect, type, fmt, ...)` | Log-style print into a section. |
| `kprintf(sect, color, fmt, ...)` | Colored print into a section. |
| `kprintf(sect, type, color, ...)` | Log-style + Colored print into a section. |

---

## Helper Functions (vga Namespace)
The `vga` namespace contains low-level screen management utilities.

### Creating Sections
* `create_section(startX, startY, endX, endY)`: Creates a section using raw integers.
* `create_section(start_coords, end_coords)`: Creates a section using coordinate structs.

### Cursor Management
* `update_cursor()`: Syncs the cursor to the current text position.
* `set_cursor_updatability(bool)`: Enables or disables automatic cursor updates.

### Screen Clearing & Manipulation
* `clear_screen()`: Wipes the entire buffer and resets the cursor.
* `clear_text_region(col, row, len)`: Clears a specific line segment.
* `backspace()`: Moves the cursor back one position and clears the character.

### Direct Insertion
* `insert(col, row, color, update_pos, fmt, ...)`: Inserts text directly at a specific coordinate without affecting the global cursor flow.

## Low-Level Primitives
Direct character output functions used by the formatted printers.

* `kputchar(color, char)`: Prints a single character.
* `kputs(color, str)`: Prints a raw string.
