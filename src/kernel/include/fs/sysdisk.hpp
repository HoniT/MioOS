// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#ifndef SYSDISK_HPP
#define SYSDISK_HPP

#include <kernel_main.hpp>

namespace sysdisk {
    void get_sysdisk(const multiboot_info* mbi);
} // namespace sysdisk

#endif // SYSDISK_HPP
