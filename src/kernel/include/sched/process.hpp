// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <stdint.h>
#include <x86/sched/context.hpp>
#include <lib/data/list.hpp>

#define KERNEL_PROCESS_STACK_SIZE 8192
#define KERNEL_PROCESS_EFLAGS 0x202

#define TIME_QUANTUM 5

#define PROCESS_MIN_PRIORITY 1
#define PROCESS_MAX_PRIORITY 10

#define KERNEL_ERROR_PID 0xFFFFFFFF

enum ProcessState {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
};

struct context_t;
struct pd_t;

class Process {
    private:
    // Process information
    uint32_t pid;
    const char* name;
    ProcessState state;
    
    context_t ctx;
    void* stack;
    pd_t* pd;
    
    uint32_t priority;
    uint32_t time_slice;
    
    public:
    // Creates a process
    static Process* create(void (*entry)(), uint32_t priority, const char* name = "");
    Process() 
    : pid(KERNEL_ERROR_PID), stack(nullptr), pd(nullptr), name(""), state(PROCESS_READY),
    priority(PROCESS_MIN_PRIORITY), time_slice(TIME_QUANTUM) { }
    
    void start(void);
    void exit(void);
    static void yield(void);
    
    // Getters
    context_t* get_ctx();
    void* get_stack();
    pd_t* get_pd();
    uint32_t get_pid();
    uint32_t get_priority();
    uint32_t get_time_slice();
    const char* get_name();
    ProcessState get_state();
    
    // Setters
    void set_time_slice();
    void decrement_time_slice();
    void set_state(ProcessState state);
    void set_priority(uint8_t p);
};

extern data::list<Process*> process_log_list;;

#endif // PROCESS_HPP
