// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef CPU_HPP
#define CPU_HPP

namespace cpu
{
    /// @brief Stops the cpu forever
    [[noreturn]] void halt();

    /// @brief Enables interrupts
    void enable_interrupts();

    /// @brief Disables interrupts
    void disable_interrupts();
} // namespace cpu


#endif // CPU_HPP
