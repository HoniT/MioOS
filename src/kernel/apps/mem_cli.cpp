// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// mem_cli.cpp
// Memory CLI commands
// ========================================

#include <apps/mem_cli.hpp>
#include <apps/kterminal.hpp>
#include <mm/heap.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <lib/math.hpp>
#include <lib/data/list.hpp>
#include <lib/data/string.hpp>

void cmd::mem_cli::register_app() {
    cmd::register_command("heapinfo", heapdump, "", " - Prints kernel heap info");
    cmd::register_command("meminfo", meminfo, "", " - Prints system memory info");
}

static void draw_memory_bar(uint64_t used, uint64_t total) {
    const int BAR_WIDTH = 40;
    // Calculate percentage using integer math (0-100)
    uint64_t percentage = (total > 0) ? udiv64(used * 100, total) : 0;
    int filled_chars = udiv64(used * BAR_WIDTH, total);

    kprintf("[");
    for (int i = 0; i < BAR_WIDTH; i++) {
        if (i < filled_chars) kprintf("#");
        else kprintf("-");
    }
    kprintf("] %llu%%\n", percentage);
}

/// @brief Prints kernel heap status
void cmd::mem_cli::heapdump() {
    // No parameters expected
    if(cmd::mem_cli::get_params().count() != 0) {
        kprintf("heap_dump: Syntax: heapdump\n");
        return;
    }

    HeapBlock* current = heap::heap_head; // Setting the current block as the head
    uint64_t bytes_in_use = 0;
    uint32_t allocated_block_num = 0;

    // Itterating through blocks
    while(current) {
        if(!current->free) bytes_in_use += current->size;
        // Getting next block
        current = current->next;
    }
    // Printing final status of heap
    kprintf("\n--- Heap Memory Usage ---\n");
    kprintf("Heap size: %S\n", get_units(HEAP_SIZE));
    kprintf("Heap status: %S used\n", get_units(bytes_in_use));
    draw_memory_bar(bytes_in_use, HEAP_SIZE);
}

static void print_meminfo(bool verbose) {
    uint64_t free_ram = pmm::total_usable_ram - pmm::total_used_ram;

    kprintf("\n--- Physical Memory Usage ---\n");
    
    kprintf("Total Installed:  %S\n", get_units(pmm::total_installed_ram));
    kprintf("Usable RAM:       %S\n", get_units(pmm::total_usable_ram));
    kprintf("Used RAM:         %S\n", get_units(pmm::total_used_ram));
    kprintf("Free RAM:         %S\n", get_units(free_ram));

    draw_memory_bar(pmm::total_used_ram, pmm::total_usable_ram);

    if (verbose) {
        kprintf("\n--- Advanced Details ---\n");
        kprintf("Hardware Reserved: %S\n", get_units(pmm::hardware_reserved_ram));
        kprintf("Kernel physical start address: %x\n", pmm::get_kernel_addr());
        kprintf("Kernel physical end address: %x\n", pmm::get_kernel_end());
        kprintf("Kernel size: %S\n", get_units(pmm::get_kernel_size()));
        kprintf("\n");
        kprintf("Kernel's active Page Directory: %x\n", vmm::get_active_pd());
        kprintf("Paging status: %s PAE status: %s\n", vmm::enabled_paging ? "Enabled" : "Disabled",
            vmm::pae_paging ? "Enabled" : "Disabled");
        kprintf("\n");
        kprintf("Kernel heap start address: %x\n", HEAP_START);
        kprintf("Kernel heap size: %S\n", get_units(HEAP_SIZE));
    }
    kprintf("\n");
}

/// @brief Displays memory information
void cmd::mem_cli::meminfo() {
    data::list<data::string> params = cmd::mem_cli::get_params();

    // 1. Help Check
    if (params.count() > 0 && (params.at(0) == "-h" || params.at(0) == "--help")) {
        kprintf("Usage: meminfo <flag>\n");
        kprintf("Flags:\n");
        kprintf("  -v, --verbose    Display detailed hardware and kernel reservations\n");
        kprintf("  --mmap           Displays memory map\n");
        kprintf("  -h, --help       Show this help message\n");
        return;
    }

    // 2. Argument Validation
    if (params.count() > 1) {
        kprintf("meminfo: too many arguments. Try 'meminfo --help'\n");
        return;
    }

    // 3. Execution
    bool verbose = false;
    if (params.count() == 1) {
        if (params.at(0) == "-v" || params.at(0) == "--verbose") {
            verbose = true;
        } 
        else if (params.at(0) == "--mmap") {
            pmm::print_memory_map();
            return;
        }
        else {
            kprintf("meminfo: invalid flag \"%S\". Try -h\n", params.at(0));
            return;
        }
    }

    print_meminfo(verbose);
}