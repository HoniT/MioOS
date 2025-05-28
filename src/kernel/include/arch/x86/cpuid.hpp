// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef CPUID_HPP
#define CPUID_HPP

#include <stdint.h>

// Struct to return every needed register
struct CPUIDResult {
    uint32_t eax;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
};

// List of CPUID functions
enum CPUID_Functions {
    CPUID_VENDOR_STRING          = 0,  // Returns the vendor string in EBX, EDX, ECX
    CPUID_FEATURES               = 1,  // Returns CPU features in EDX and ECX
    CPUID_CACHE_TLB_INFO         = 2,  // Cache and TLB information
    CPUID_SERIAL_NUMBER          = 3,  // Processor serial number (if supported)
    CPUID_CACHE_TOPOLOGY         = 4,  // Cache topology information (requires ECX input)
    CPUID_MONITOR_INFO           = 5,  // Monitor/MWAIT features
    CPUID_THERMAL_POWER          = 6,  // Thermal and power management features
    CPUID_EXTENDED_FEATURES      = 7,  // Extended features (pass ECX=0 for subleaf 0)
    CPUID_PERFMON_INFO           = 10, // Performance monitoring features
    CPUID_EXTENDED_FUNCTIONS     = 0x80000000, // Maximum supported extended CPUID leaf
    CPUID_EXTENDED_PROCESSOR_INFO = 0x80000001, // Extended CPU features
    CPUID_BRAND_STRING_1         = 0x80000002, // Processor brand string (part 1)
    CPUID_BRAND_STRING_2         = 0x80000003, // Processor brand string (part 2)
    CPUID_BRAND_STRING_3         = 0x80000004, // Processor brand string (part 3)
    CPUID_L1_CACHE_INFO          = 0x80000005, // L1 cache and TLB information
    CPUID_L2_CACHE_INFO          = 0x80000006, // L2 cache and TLB information
    CPUID_EXTENDED_CACHE_INFO    = 0x80000007, // Advanced power management information
    CPUID_VIRTUALIZATION_INFO    = 0x80000008  // Virtualization and address size
};


extern char cpu_vendor[13]; // Vendor name
extern char cpu_model_name[49];  // Model name

namespace cpu {
    void get_processor_info(void);
    void get_vendor(char* vendor);
    void get_processor_model(char* buffer);
    CPUIDResult cpuid(const uint32_t eax_input);
} //namespace cpu


#endif // CPUID_HPP
