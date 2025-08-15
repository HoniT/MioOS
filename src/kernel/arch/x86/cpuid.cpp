// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// cpuid.cpp
// In charge of CPUID functions
// ========================================

#include <cpuid.hpp>
#include <drivers/vga_print.hpp>

// Gets the processor model
void cpu::get_processor_model(char* buffer) {
    CPUIDResult result;

    // Get brand string (12 chars each from 0x80000002, 0x80000003, 0x80000004)
    result = cpu::cpuid(CPUID_BRAND_STRING_1);
    *(uint32_t*)&buffer[0]  = result.eax;
    *(uint32_t*)&buffer[4]  = result.ebx;
    *(uint32_t*)&buffer[8]  = result.ecx;
    *(uint32_t*)&buffer[12] = result.edx;

    result = cpu::cpuid(CPUID_BRAND_STRING_2);
    *(uint32_t*)&buffer[16] = result.eax;
    *(uint32_t*)&buffer[20] = result.ebx;
    *(uint32_t*)&buffer[24] = result.ecx;
    *(uint32_t*)&buffer[28] = result.edx;

    result = cpu::cpuid(CPUID_BRAND_STRING_3);
    *(uint32_t*)&buffer[32] = result.eax;
    *(uint32_t*)&buffer[36] = result.ebx;
    *(uint32_t*)&buffer[40] = result.ecx;
    *(uint32_t*)&buffer[44] = result.edx;

    buffer[48] = '\0';  // Null-terminate the string
}

// Main function for CPUID operations
void cpu::get_vendor(char* vendor) {
    CPUIDResult result = cpuid(CPUID_VENDOR_STRING);

    *(uint32_t*)(&vendor[0]) = result.ebx; // EBX contains first 4 bytes
    *(uint32_t*)(&vendor[4]) = result.edx; // EDX contains next 4 bytes
    *(uint32_t*)(&vendor[8]) = result.ecx; // ECX contains last 4 bytes
    vendor[12] = '\0'; // Null terminate the string

    return;
}

char cpu_vendor[13]; // Vendor name
char cpu_model_name[49];  // Model name
// Gets basic info sutch as model and vendor saves and prints it
void cpu::get_processor_info(void) {
    // Getting vender
    get_vendor(cpu_vendor);
    // Getting model
    get_processor_model(cpu_model_name);
    return;
}

/* Takes in a parameter for the function number and returns 
 * EAX, EBX, EDX, ECX */
CPUIDResult cpu::cpuid(const uint32_t eax_input) {
    CPUIDResult result;

    asm volatile(
        "cpuid"
        : "=a"(result.eax), "=b"(result.ebx), "=c"(result.ecx), "=d"(result.edx) // Outputs
        : "a"(eax_input), "c"(0)                                                // Inputs
    );

    return result;
}
