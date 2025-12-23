// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// process.cpp
// Manages processs
// ========================================

#include <sched/process.hpp>
#include <sched/scheduler.hpp>
#include <x86/sched/context.hpp>
#include <mm/heap.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <graphics/vga_print.hpp>
#include <lib/data/queue.hpp>
#include <lib/string_util.hpp>
#include <lib/math.hpp>

template <typename Func>
inline void atomic_procedure(Func function) {
    asm volatile("cli");
    function();
    asm volatile("sti");
}

static void* alloc_kernel_process_stack() {
    // Rounding up to multiples of FRAME_SIZE
    uint32_t blocks = ((KERNEL_PROCESS_STACK_SIZE + FRAME_SIZE - 1) & ~(FRAME_SIZE - 1)) / FRAME_SIZE;
    // Just return identity mapped address
    return pmm::alloc_frame(blocks);
}

static uint32_t next_pid = 0;
uint32_t alloc_pid() { return next_pid++; }

/// @brief Creates a kernel process
/// @param entry Function that the process will do
/// @param priority Process priority 1-10
Process* Process::create(void (*entry)(), uint32_t priority, const char* process_name) {
    Process* proc = (Process*)kcalloc(1, sizeof(Process));

    proc->pid = alloc_pid();
    proc->name = (strcmp(process_name, "") == 0) ? 
        strcat("Kernel Process ", num_to_string(proc->pid)) : process_name;

    // Putting valid priority
    priority = range(priority, PROCESS_MIN_PRIORITY, PROCESS_MAX_PRIORITY);
    
    proc->state = PROCESS_READY;
    proc->priority = priority;
    // Weighted round robin
    proc->time_slice = TIME_QUANTUM * priority;

    proc->pd = vmm::get_active_pd();
    proc->ctx.cr3 = (uint32_t)vmm::virtual_to_physical((uint32_t)proc->pd);

    // Allocate stack
    void* stack_bottom = alloc_kernel_process_stack();
    if(!stack_bottom) {
        kprintf(LOG_ERROR, "Couldn't allocate stack for %s (PID: %u)\n", 
                proc->name, proc->pid);
        return nullptr;
    }
    proc->stack = stack_bottom;
    
    // Calculate stack top (stack grows downward)
    uint32_t stack_top = (uint32_t)stack_bottom + KERNEL_PROCESS_STACK_SIZE;

    // AUTOMATIC EXIT
    stack_top -= sizeof(uint32_t); 
    *((uint32_t*)stack_top) = (uint32_t)sched::exit_current_process;
    
    // Set context registers
    proc->ctx.esp = stack_top;
    proc->ctx.eip = (uint32_t)entry;
    proc->ctx.eflags = KERNEL_PROCESS_EFLAGS;
    proc->ctx.cs = 0x08;
    proc->ctx.ds = 0x10;
    proc->ctx.es = 0x10;
    proc->ctx.fs = 0x10;
    proc->ctx.gs = 0x10;
    proc->ctx.ss = 0x10;
    // Initialize general purpose registers to zero
    proc->ctx.eax = 0;
    proc->ctx.ebx = 0;
    proc->ctx.ecx = 0;
    proc->ctx.edx = 0;
    proc->ctx.esi = 0;
    proc->ctx.edi = 0;
    proc->ctx.ebp = 0;

    return proc;
}

/// @brief Starts executing a kernel process
void Process::start(void) {
    this->state = PROCESS_READY;

    // Adding to process queue, scheduler will do the rest
    atomic_procedure([this](){
        process_queue.push(this);
    });
}

void Process::exit(void) {
    // IMPORTANT: Do NOT free the stack here. We are currently running on it!
    // The scheduler will handle the cleanup (Zombie Reaping).
    this->state = PROCESS_TERMINATED;
    
    // Trigger reschedule - will never return
    sched::schedule();
    
    // Should never reach here
    for(;;) asm volatile("hlt");
}

void Process::yield(void) {
    // Voluntarily give up CPU
    sched::schedule();
}

// Getters

context_t* Process::get_ctx() { return &this->ctx; }
void* Process::get_stack() { return this->stack; }
pd_t* Process::get_pd() { return this->pd; }
uint32_t Process::get_pid() { return this->pid; }
uint32_t Process::get_priority() { return this->priority; }
uint32_t Process::get_time_slice() { return this->time_slice; }
const char* Process::get_name() { return this->name; }
ProcessState Process::get_state() { return this->state; }

void Process::set_time_slice() { this->time_slice = TIME_QUANTUM * this->priority; }
void Process::decrement_time_slice() { if(this->time_slice > 0) this->time_slice--; }
void Process::set_state(ProcessState state) { this->state = state; }
void Process::set_priority(uint8_t p) { priority = p; }
