// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <sched/process.hpp>
#include <lib/data/queue.hpp>

extern Process* curr_process;
extern data::queue<Process*> process_queue;

struct InterruptRegisters;
namespace sched {
    void init();
    void exit_current_process();
    void zombie_reaper();
    void schedule();
} // namespace sched

#endif // SCHEDULER_HPP
