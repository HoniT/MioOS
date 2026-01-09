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
#include <mm/pmm.hpp>

Process* curr_process;
data::queue<Process*> process_queue;
static data::queue<Process*> zombie_queue; // Processes waiting to be reaped

/// Idle process used to have a valid curr_process when nothing else runs
static void kernel_idle(void) {
    for (;;) {
        asm volatile("sti"); 
        asm volatile("hlt");
    }
}

void dump_process(Process process);

static Process* kernel_idle_process;
/// @brief Initializes scheduler
void sched::init() {
    // Creating kernel idle process to keep scheduler busy
    kernel_idle_process = Process::create(kernel_idle, 1, "Kernel Idle Process");
    curr_process = kernel_idle_process;
    curr_process->start();

    Process* zombie_reaper = Process::create(sched::zombie_reaper, 1, "Zombie Process Reaper");
    zombie_reaper->start();
    
    if(!curr_process || curr_process->get_pid() == KERNEL_ERROR_PID) {
        kprintf(LOG_ERROR, "Failed to initialize Scheduler! (Couldn't create kernel idle process)\n");
        kernel_panic("Fatal component failed to initialize!");
    }
    else kprintf(LOG_INFO, "Implemented Scheduler\n");
    sched::schedule();
}

/// @brief Terminates zombie processes, will run on seperate kernel thread
void sched::zombie_reaper() {
    while (true) {
        Process* z = nullptr;
        asm volatile("cli");
        if (!zombie_queue.empty()) {
            z = zombie_queue.pop();
        }
        asm volatile("sti");

        // If we found a zombie, free it
        if (z) {
            // kprintf(RGB_COLOR_GREEN, "Reaping process %u (%s)\n", z->get_pid(), z->get_name());
            if (z->get_stack()) {
                pmm::free_frame(z->get_stack());
            }
            kfree(z);
        } 
        else {
            Process::yield();
        }
    }
}

void sched::exit_current_process() {
    if (!curr_process || curr_process->get_pid() == 0) {
        return; // Can't exit idle process
    }

    // Just call exit on the process, it handles state change and schedule call
    curr_process->exit();
}

void sched::schedule() {
    if(!curr_process) {
        return;
    }

    Process* old_process = curr_process;
    Process* next = nullptr;

    // 2. SELECT NEXT PROCESS
    if (!process_queue.empty()) {
        // Standard Round Robin: Pop the head. 
        // We will push old_process to the tail later if it's still running.
        next = process_queue.pop(); 
    }

    // 3. HANDLE NO NEXT PROCESS FOUND
    if (!next) {
        // If queue is empty, checks if the OLD process can still run
        if (old_process->get_state() == PROCESS_RUNNING && old_process != kernel_idle_process) {
            old_process->set_time_slice();
            return;
        }
        
        next = kernel_idle_process;
    }

    // 4. UPDATE OLD PROCESS STATE
    if(old_process->get_state() == PROCESS_RUNNING) {
        // If it was running, and we are switching away, put it back in queue (Tail)
        if (old_process != next && old_process != kernel_idle_process) {
            process_queue.push(old_process);
        }
    }
    else if (old_process->get_state() == PROCESS_TERMINATED) {
        // Mark for reaping
        zombie_queue.push(old_process);
    }

    // 5. CONTEXT SWITCH
    next->set_state(PROCESS_RUNNING);
    next->set_time_slice();
    curr_process = next;

    if (old_process != next) {
        ctx_switch(old_process->get_ctx(), next->get_ctx());
    }
}
