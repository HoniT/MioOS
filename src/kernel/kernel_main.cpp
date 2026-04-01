#include <stdint.h>

extern "C" void kernel_main() {
    // Physical VGA buffer is at 0xB8000
    // Higher-half virtual address is 0xFFFFFFFF80000000 + 0xB8000
    uint16_t* vga_buffer = (uint16_t*)0xFFFFFFFF800B8000;
    
    const char* str = "Successfully booted into 64-bit Higher-Half Long Mode!";
    
    for (int i = 0; str[i] != '\0'; ++i) {
        // Character | Color (White text [15] on Black background [0])
        vga_buffer[i] = (uint16_t)str[i] | ((uint16_t)15 << 8); 
    }

    while (1) {
        __asm__ volatile("hlt");
    }
}