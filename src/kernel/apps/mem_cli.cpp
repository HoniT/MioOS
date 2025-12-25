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
#include <lib/math.hpp>
#include <lib/data/list.hpp>
#include <lib/data/string.hpp>

void cmd::mem_cli::register_app() {
    cmd::register_command("heapinfo", heapdump, "", " - Prints kernel heap info");
    cmd::register_command("meminfo", meminfo, "", " - Prints system memory info");
}

/// @brief Prints kernel heap status
void cmd::mem_cli::heapdump() {
    // No parameters expected
    if(cmd::mem_cli::get_params().count() != 0) {
        kprintf("heap_dump: Syntax: heapdump\n");
        return;
    }

    kprintf("--------------- Heap Dump ---------------\n");

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
    kprintf("Heap status: %llu bytes used out of %lu (%u%%)\n", bytes_in_use, HEAP_SIZE, udiv64(bytes_in_use * 100, HEAP_SIZE));
    kprintf("-----------------------------------------\n");
}

static void basic_meminfo() {
    kprintf("Use flag \"-h\" to get every specific version of getmeminfo.\n");
    kprintf("Total installed memory:  %llu bytes (~%llu GiB)\n", pmm::total_installed_ram, pmm::total_installed_ram / BYTES_IN_GIB);
    kprintf("Available usable memory: %llu bytes (~%llu GiB)\n", pmm::total_usable_ram, pmm::total_usable_ram / BYTES_IN_GIB);
    kprintf("Used memory:                %llu bytes\n", pmm::total_used_ram);
}

static void advanced_meminfo() {
    kprintf("Use flag \"-h\" to get evry specific version of getmeminfo.\n");
    kprintf("Total installed memory:  %llu bytes (~%llu GiB)\n", pmm::total_installed_ram, pmm::total_installed_ram / BYTES_IN_GIB);
    kprintf("Available usable memory: %llu bytes (~%llu GiB)\n", pmm::total_usable_ram, pmm::total_usable_ram / BYTES_IN_GIB);
    kprintf("Used memory:                %llu bytes\n", pmm::total_used_ram);
    kprintf("Hardware reserved memory:   %llu bytes\n", pmm::hardware_reserved_ram);
}

/// @brief Displays memory information
void cmd::mem_cli::meminfo() {
    // Managing parameters
    data::list<data::string> params = cmd::mem_cli::get_params();

    // Displaying basic memory info
    if(params.count() == 0) {
        basic_meminfo();
        return;
    }
    
    // To many params
    if(params.count() > 1) {
        kprintf("meminfo: Syntax: meminfo <flag>");
        return;
    }

    if(params.at(0) == "-v" || params.at(0) == "--verbose") {
        advanced_meminfo();
        return;
    }
    else if(params.at(0) == "-h" || params.at(0) == "--help") {
        kprintf("-v/--verbose - Prints advanced info\n");
    }
    else {
        kprintf("Invalid flag \"%S\", try -h/--help for available flags\n", params.at(0));
    }
}
