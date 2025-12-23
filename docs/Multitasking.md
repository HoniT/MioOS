# System Documentation & Guides

This document contains technical documentation for the multitasking scheduler, as well as user guides for running the operating system on real hardware and maintaining installation media.

---

# Part 1: Multitasking & Scheduler Documentation

## Overview

The `mio_os` multitasking system implements a **preemptive, priority-aware Round Robin scheduler**. It allows multiple kernel threads to run concurrently by time-slicing the CPU.

The system is composed of three main components:
1.  **Process Manager (`Process`):** Handles creation, stack allocation, and state management of individual execution contexts.
2.  **Scheduler (`sched`):** Decides which process runs next and performs the actual context switch.
3.  **Timer Interrupt (PIT):** Provides the "ticks" that trigger preemptive task switching.

## 1. Process Lifecycle

Every task in the system is represented by a `Process` structure. Processes move through specific states during their lifetime.

### States
* **READY:** The process is initialized and waiting in the `process_queue` for CPU time.
* **RUNNING:** The process is currently executing on the CPU.
* **BLOCKED:** (Reserved) The process is waiting for an I/O event or lock.
* **TERMINATED:** The process has finished execution and is waiting for its resources (stack) to be freed by the Zombie Reaper.

### Process Creation
When `Process::create()` is called:
1.  A `Process` struct is allocated on the kernel heap.
2.  A dedicated **8KB stack** is allocated via the Physical Memory Manager (PMM).
3.  **Automatic Exit Setup:** A "return address" pointing to `sched::exit_current_process` is manually pushed onto the stack. This ensures that if a process function returns (e.g., `return;` or `}`), it gracefully exits instead of crashing.
4.  CPU registers (`EIP`, `ESP`, `EFLAGS`) are initialized.

## 2. Scheduling Algorithm

The scheduler uses a **Weighted Round Robin** approach.

### The Queue
* **`process_queue`:** A FIFO queue holding all `READY` processes.
* **`kernel_idle_process`:** A special infinite loop process that runs only when the `process_queue` is empty to prevent the CPU from halting.

### Time Slicing & Priority
Each process is assigned a **Time Quantum** based on its priority (1-10).

$$\text{Time Slice} = \text{Base Quantum (5 ticks)} \times \text{Priority}$$

* **Higher Priority:** Longer time slice (runs longer before being interrupted).
* **Lower Priority:** Shorter time slice.

### The Context Switch Flow
1.  **Trigger:** The PIT (Programmable Interval Timer) fires IRQ0 every 1ms.
2.  **Decrement:** The current process's `time_slice` is decremented.
3.  **Preemption:** If `time_slice == 0`, `sched::schedule()` is called.
4.  **Switching:**
    * The current process is moved to the back of the `process_queue` (if still running).
    * The next process is popped from the front of the queue.
    * `ctx_switch()` saves the old registers and loads the new ones.

## 3. Zombie Reaping

Processes cannot free their own stack memory while they are running on it. To solve this, we use a **Zombie Reaper** strategy.

1.  **Termination:** When a process calls `exit()`, it sets its state to `TERMINATED` and yields the CPU.
2.  **Zombie Queue:** The scheduler detects the `TERMINATED` state and pushes the process into a `zombie_queue` instead of the run queue.
3.  **The Reaper Process:** A dedicated background process (`sched::zombie_reaper`) wakes up periodically, checks the `zombie_queue`, and safely frees the memory of dead processes.

## 4. API Reference

### Process Management

```cpp
// Create a new kernel process
// priority: 1 (lowest) to 10 (highest)
Process* proc = Process::create(my_function, 5, "My Process");

// Start the process (adds it to the scheduler queue)
proc->start();

// Voluntarily give up the CPU (e.g., while waiting)
Process::yield();
```

### Scheduler Control

```cpp
// Initialize the scheduler (Must be called after IDT/GDT/PMM)
sched::init();

// Force a context switch manually
sched::schedule();

// Exit the current process (called automatically on return)
sched::exit_current_process();
```

### Technical specifications

| Parameter | Value | Description |
| :--- | :--- | :--- |
| **Stack Size** | 8192 Bytes (8KB) | Per-process kernel stack. |
| **Base Quantum** | 5 Ticks | 1 tick ≈ 1ms (1000Hz). |
| **Max Priority** | 10 | Max slice = 50ms. |
| **Min Priority** | 1 | Min slice = 5ms. |
| **Scheduler Type** | Preemptive | Round Robin with Priorities. |
