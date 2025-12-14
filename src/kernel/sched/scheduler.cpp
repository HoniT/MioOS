// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// scheduler.cpp
// Manages scheduling processs
// ========================================

#include <sched/scheduler.hpp>
#include <x86/sched/context.hpp>
#include <x86/interrupts/idt.hpp>
#include <x86/interrupts/kernel_panic.hpp>
#include <graphics/vga_print.hpp>
#include <drivers/vga.hpp>
#include <mm/pmm.hpp> // Required for pmm::free_frame

// Helper funcitons

template <typename Func>
inline void atomic_procedure(Func function) {
    asm volatile("cli");
    function();
    asm volatile("sti");
}

Process* curr_process;
data::queue<Process*> process_queue;
static Process* zombie_process = nullptr; // Process waiting to be reaped

/// Idle process used to have a valid curr_process when nothing else runs
static void kernel_idle(void) {
    for (;;) {
        // Must enable interrupts so the Timer IRQ can wake us up to switch tasks!
        asm volatile("sti"); 
        asm volatile("hlt");
    }
}

void dump_process(Process process);

static Process* kernel_idle_process;
/// @brief Initializes scheduler
void sched::init() {
    // Creating kernel idle process to keep scheduler busy
    kernel_idle_process = Process::create(kernel_idle, 0, "Kernel Idle Process");
    curr_process = kernel_idle_process;
    // curr_process->start();

    if(!curr_process || curr_process->get_pid() == KERNEL_ERROR_PID) {
        kprintf(LOG_ERROR, "Failed to initialize Scheduler! (Couldn't create kernel idle process)\n");
        kernel_panic("Fatal component failed to initialize!");
    }
    else kprintf(LOG_INFO, "Implemented Scheduler\n");
}

void sched::exit_current_process() {
    if (!curr_process || curr_process->get_pid() == 0) {
        return; // Can't exit idle process
    }
    
    // Just call exit on the process, it handles state change and schedule call
    curr_process->exit();
}

void sched::schedule() {
    // 1. REAP ZOMBIES
    // If we have a process that died in the previous switch, free it now
    if (zombie_process) {
        if (zombie_process->get_stack()) {
             pmm::free_frame(zombie_process->get_stack());
        }
        kfree(zombie_process);
        zombie_process = nullptr;
    }

    if(!curr_process) {
        return;
    }

    Process* old_process = curr_process;
    // If empty continue with old task (unless it terminated)
    if(process_queue.empty()) {
        if(old_process->get_state() == PROCESS_RUNNING) {
            old_process->set_time_slice();
            return; 
        } else if (old_process->get_state() == PROCESS_TERMINATED) {
             // Terminated but queue empty, switch to idle
        } else {
             return;
        }
    }

    // Finding next valid process by priotrity
    data::queue<Process*> temp_queue;
    uint32_t highest_priority = 0;
    Process* next = nullptr;

    while (!process_queue.empty()) {
        Process* p = process_queue.pop();

        // Skip terminated processes that might be lingering
        if (p->get_state() == PROCESS_TERMINATED) {
            // Free the process
            if (p->get_stack()) pmm::free_frame(p->get_stack());
            kfree(p);
            continue;
        }

        temp_queue.push(p);
        
        if (p->get_priority() >= highest_priority) {
            highest_priority = p->get_priority();
            next = p;
        }
    }
    // Restore queue and remove selected task
    while (!temp_queue.empty()) {
        Process* p = temp_queue.pop();
        if (p != next) {
            process_queue.push(p);
        }
    }

    // If no valid process found, switch to idle
    if (!next) {
        // If old_process is running, just keep running it
        if (old_process->get_state() == PROCESS_RUNNING && old_process != kernel_idle_process) {
             old_process->set_time_slice();
             return;
        }

        kprintf(LOG_INFO, "[SCHED] No runnable processes, switching to idle\n");
        
        next = kernel_idle_process;
        next->set_state(PROCESS_RUNNING);
    }

    // Updating states
    if(old_process->get_state() == PROCESS_RUNNING && old_process != next) {
        // Not adding kernel idle
        if(old_process->get_pid() != 0) {
            old_process->set_state(PROCESS_READY);
            process_queue.push(old_process);
        }
    }
    else if (old_process->get_state() == PROCESS_TERMINATED) {
        // Mark for reaping on next schedule
        zombie_process = old_process;
    }

    next->set_state(PROCESS_RUNNING);
    next->set_time_slice();
    curr_process = next;

    // Perform context switch
    if (old_process != next) {
        ctx_switch(old_process->get_ctx(), next->get_ctx());
    }
}