// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <kernel_main.hpp>

extern "C" void kernel_main(void* mbi, uint32_t magic) {
    uint16_t* vga_buffer = (uint16_t*)0xFFFFFFFF800B8000;
    
    const char* str = "Successfully booted into 64-bit Higher-Half Long Mode!";
    
    for (int i = 0; str[i] != '\0'; ++i) {
        // Character | Color (White text [15] on Black background [0])
        vga_buffer[i] = (uint16_t)str[i] | ((uint16_t)15 << 8); 
    }
    for(;;) asm volatile("hlt");
}