// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// pmm.cpp
// Is the Physical Memory Manager, allocates/frees frames...
// ========================================

#include <mm/pmm.hpp>
#include <mm/heap.hpp>
#include <mm/vmm.hpp>
#include <kernel_main.hpp>
#include <graphics/vga_print.hpp>
#include <kterminal.hpp>
#include <x86/interrupts/kernel_panic.hpp>
#include <lib/math.hpp>
#include <lib/mem_util.hpp>
#include <lib/string_util.hpp>

// Getting the kernels physical starting address from the linker script
extern "C" uint32_t __kernel_phys_base;

// Amount of certain types of memory
uint64_t pmm::total_usable_ram = 0;
uint64_t pmm::total_used_ram = 0;
uint64_t pmm::hardware_reserved_ram = 0;
uint64_t pmm::total_installed_ram = 0;

// Multiboot info will be saved here
void* mb2_info = nullptr;

uint64_t LOW_DATA_START_ADDR = 0;
uint64_t metadata_reserved = 0; // Space reserved by frame metadata

// We will have two different heads for two allocatable regions 
MetadataNode* pmm::low_alloc_mem_head = nullptr; // Lower allocatable RAM: usually 0x100000 - 0xBFFDFFFF
MetadataNode* pmm::high_alloc_mem_head = (MetadataNode*)(METADATA_ADDR + sizeof(MetadataNode)); // High allocatable RAM: usually 4 GiB and onwards

#pragma region Memory Map Manager

// Prints out the memory map: debugging / helper function
void pmm::print_memory_map(void) {
    if (!mb2_info) {
        kprintf(LOG_ERROR, "Memory map not defined\n");
        return;
    }

    // Get the memory map tag
    multiboot_tag_mmap* mmap_tag = Multiboot2::get_mmap(mb2_info);
    if (!mmap_tag) {
        kprintf(LOG_ERROR, "Memory map tag not found\n");
        return;
    }

    // Iterate over the memory map entries
    uint32_t entry_count = (mmap_tag->size - sizeof(multiboot_tag_mmap)) / mmap_tag->entry_size;
    
    for (uint32_t i = 0; i < entry_count; i++) {
        multiboot_mmap_entry* entry = &mmap_tag->entries[i];
        
        // Calculate the end address
        uint64_t end_addr = entry->addr + entry->len - 1;

        // Determine type string
        const char* type_str;
        switch(entry->type) {
            case MULTIBOOT_MEMORY_AVAILABLE:
                type_str = "Available";
                break;
            case MULTIBOOT_MEMORY_RESERVED:
                type_str = "Reserved";
                break;
            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                type_str = "ACPI Reclaimable";
                break;
            case MULTIBOOT_MEMORY_NVS:
                type_str = "ACPI NVS";
                break;
            case MULTIBOOT_MEMORY_BADRAM:
                type_str = "Bad RAM";
                break;
            default:
                type_str = "Unknown";
                break;
        }

        // Print memory region information
        kprintf("Memory region: %llx - %llx, Type: %s\n",
                   entry->addr, end_addr, type_str);
    }
}

// Prints info of blocks in the allocatable memory regions 
void pmm::print_usable_regions(void) {
    // Getting list head
    MetadataNode* current = low_alloc_mem_head;

    // Iterating through blocks
    while(current) {
        // Printing info
        kprintf("-Block range: %lx-%lx\n", current->addr, current->addr + current->size - 1);
        kprintf("Block size: %lx\n", current->size);
        kprintf("Blocks of memory taken up: %d\n", udiv64(current->size, FRAME_SIZE));
        kprintf("Block status: %s\n", (current->free) ? "Free" : "Allocated");

        // Going to next block
        current = current->next;
    }
}

// Gets the total amount of usable RAM in the system and creates head (and other) nodes for memory alloc
void pmm::manage_mmap(void* _mb2_info) {
    // Get the memory map tag
    multiboot_tag_mmap* mmap_tag = Multiboot2::get_mmap(_mb2_info);
    if (!mmap_tag) {
        kprintf(LOG_ERROR, "Memory map not available in Multiboot2 info!\n");
        return;
    }

    // Calculate number of entries
    uint32_t entry_count = (mmap_tag->size - sizeof(multiboot_tag_mmap)) / mmap_tag->entry_size;

    // Iterate through mmap entries
    for (uint32_t i = 0; i < entry_count; i++) {
        multiboot_mmap_entry* entry = &mmap_tag->entries[i];

        // If the mmap entry is available
        if(entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            
            // If the mmap entry is at 4GiB+
            if(entry->addr >= 0x100000000 && vmm::pae_paging) {
                // Keeping the total RAM amount for stats
                pmm::total_usable_ram += entry->len;

                // Manage high allocatable ram
                high_alloc_mem_head = (MetadataNode*)(METADATA_ADDR + sizeof(MetadataNode)); // The second node of the list
                high_alloc_mem_head->addr = 0x100000000; // 4GiB (High usable region start)
                high_alloc_mem_head->size = entry->len;
                high_alloc_mem_head->free = true; // Noting that this is free
                high_alloc_mem_head->next = nullptr; // Making this the linked list tail
                high_alloc_mem_head->prev = low_alloc_mem_head; // Making the low allocatable region the previous
            }
            // If the mmap entry address is at the kernels base
            else if(entry->addr == uint64_t(&__kernel_phys_base)) {
                // Keeping the total RAM amount for stats
                pmm::total_usable_ram += entry->len;

                // Creating node for low allocatable ram 
                low_alloc_mem_head = (MetadataNode*)METADATA_ADDR;
                low_alloc_mem_head->addr = 0; // We will set this later
                low_alloc_mem_head->size = entry->addr + entry->len; // We will change this later
                low_alloc_mem_head->free = true; // Noting that this is free
                // Making the high usable region the next node only if PAE paging is enabled, because
                // 32-bit paging only supports 4GiB of physical RAM
                low_alloc_mem_head->next = vmm::pae_paging ? high_alloc_mem_head : nullptr; 
                low_alloc_mem_head->prev = nullptr; // Making this the linked list head
            }
            /* There is one available mmap entry that starts at 0x0, but we will not use this because
             * to not conflict with any BIOS info/memory */
            else pmm::hardware_reserved_ram += entry->len;
        }
        // If the entry is reserved we'll add the size to the total amount of reserved ram
        else pmm::hardware_reserved_ram += entry->len;

        // Adding size to total amount of installed ram
        pmm::total_installed_ram += entry->len;
    }

    // Calculating amount of space to reserve for metadata
    metadata_reserved = vmm::pae_paging ? pmm::total_usable_ram : 0x100000000 / FRAME_SIZE * sizeof(MetadataNode);
    // Keeping the address in where the actual data will be placed and aligning it to ensure page-alignment
    LOW_DATA_START_ADDR = align_up(METADATA_ADDR + metadata_reserved, PAGE_SIZE);
}

#pragma endregion

// Initializes any additional info for the PMM
void pmm::init(void* _mb2_info) {
    // Saving multiboot2 info globally
    mb2_info = _mb2_info;
    // Calling needed functions
    pmm::manage_mmap(_mb2_info);
    
    // Setting up low_alloc_mem_head
    low_alloc_mem_head->addr = LOW_DATA_START_ADDR;
    low_alloc_mem_head->size -= LOW_DATA_START_ADDR; /* We previously set this as the mmap entry's address + size, 
    * now we'll just subtract the actual data starting address to just get the size */
    if(!low_alloc_mem_head) {
        kprintf(LOG_ERROR, "Failed to initialize physical memory manager! (Low allocable memory not defined)\n");
        kernel_panic("Fatal component failed to initialize!");
    }
   
   // Cleaning up any unwanted memory from a warm boot
   memset((void*)low_alloc_mem_head->addr, 0, low_alloc_mem_head->size);
   
   kprintf(LOG_INFO, "Implemented physical memory manager\n");
}

// Alloc and dealloc

// Allocates a frame in the usable memory regions
void* pmm::alloc_frame(const uint64_t num_blocks, bool identity_map) {
    // Precausions
    if(num_blocks <= 0) return nullptr;

    // Aligning to default frame size
    uint64_t size = num_blocks * FRAME_SIZE;

    MetadataNode* current = low_alloc_mem_head; // Setting the current block as the head

    #ifdef VMM_HPP
    // Checks if the list head is mapped
    if (vmm::enabled_paging && !vmm::is_mapped(uint32_t(low_alloc_mem_head))) {
        kprintf(LOG_ERROR, "Page fault: pmm_head is not mapped!\n");
        return nullptr;
    }
    #endif

    // Find a free block with enough space
    while(current) {
        // If the current block is free and has enough space then we can use it
        if(current->free && current->size >= size) {
            // Split the block if theres extra space
            if(current->size > size) {
                // Setting new block
                MetadataNode* new_block = (MetadataNode*)((uint8_t*)current + sizeof(MetadataNode));
                // Trimming the extra space of
                new_block->size = current->size - size;
                new_block->addr = current->addr + size; // Setting address
                new_block->free = true;
                /* Setting the current blocks next block as the trimmed (new)
                 * blocks next, because the new block is the end of the current block. Setting current as prev */
                new_block->next = current->next;
                new_block->prev = current;
                // Making the current blocks next block the new block
                current->next = new_block;
            }

            current->size = size; // Setting the passed size
            // Setting the address
            if(current->prev) 
                current->addr = current->prev->addr + current->prev->size;

            current->free = false; // Mark as allocated

            // Noting that we took up usable RAM
            pmm::total_used_ram += size;

            // Returning the current blocks address plus the metadata size needed and aligning it to the page size
            uint64_t return_address = current->addr;
            
            #ifdef VMM_HPP // If VMM is present
            // If paging is enabled
            if(vmm::enabled_paging && identity_map) {
                // Identity map the allocated frames every 4KiB block to virtual memory
                vmm::identity_map_region(return_address, return_address + PAGE_SIZE * num_blocks, PRESENT | WRITABLE);
            }
            #endif // VMM_HPP
            memset((void*)return_address, 0, FRAME_SIZE); // Zeroing out data

            return (void*)align_up(return_address, PAGE_SIZE); // Insuring alignment
        }
        // Moving to the next block
        current = current->next;
    }

    kprintf(LOG_ERROR, "Not enough memory to allocate %x block(s)!\n", num_blocks);
    return nullptr;
}

// Frees a frame
void pmm::free_frame(void* ptr) {
    if(!ptr) return;

    // Getting the frame based of of the given address/pointer and setting it as free
    MetadataNode* current = low_alloc_mem_head;
    MetadataNode* block;
    while(current) {
        // If the given address is the same as the current nodes address, that means we found it
        if(current->addr == uint64_t(ptr)) {
            // Saving the block
            block = current;
            break;
        }

        // Going to next node
        current = current->next;
    }
    block->free = true; // Setting as free
    pmm::total_used_ram -= block->size;

    loop:
    // Setting the corresponding linked list head, high or low usable mem region
    current = low_alloc_mem_head;

    bool merged_frames = false;
    // Merging adjacent free blocks to decrease fragmentation
    while(current) {
        // If the current block and the next block are free
        if(current->free && current->next && current->next->free) {
            /* We'll only merge the blocks if there is no gap between the address, we add
             * this check because we have two memory regions (lower(starting at the kernels base), and the higher(usually starting at 4GiB))
             * these two regions in the mmap have two hardware reserved regions in between,
             * to avoid overwritting these reserved regions we wont merge these two usable regions */
            if(current->next->addr == current->addr + current->size) {
                // Adding the block sizes together
                current->size += current->next->size;
                /* The current blocks next block will be the next blocks next block, 
                * because the next block doesn't exist anymore */
                current->next = current->next->next;
                if(current->next)
                    current->next->prev = current;

                merged_frames = true;
            }
        }
        // Going to the next block
        current = current->next;
    }

    // We will do this merging while every possible frame isn't merged
    if(merged_frames) goto loop;
    
    // Setting the ptr to null because it doesn't point to a valid address anymore
    ptr = nullptr;

    #ifdef VMM_HPP // If VMM is present
        // If paging is enabled
        if(vmm::enabled_paging) {
            // Unmap the allocated frames every 4KiB block from virtual memory
            for(uint64_t i = 0, addr = uint64_t(ptr); i < (block->size / PAGE_SIZE); block++, addr += PAGE_SIZE)
                vmm::free_page(uint64_t(addr));
        }
    #endif // VMM_HPP
}

#pragma region Terminal Functions

void pmm::getmeminfo(data::list<data::string> params) {
    // If there were no flags inputed
    if(params.empty()) {
        // Printing usable and used memory
        kprintf("Use flag \"-h\" to get evry specific version of getmeminfo.\n");
        kprintf("Total installed memory:  %llu bytes (~%llu GiB)\n", pmm::total_installed_ram, pmm::total_installed_ram / BYTES_IN_GIB);
        kprintf("Available usable memory: %llu bytes (~%llu GiB)\n", pmm::total_usable_ram, pmm::total_usable_ram / BYTES_IN_GIB);
        kprintf("Used memory:                %llu bytes\n", pmm::total_used_ram);
        kprintf("Hardware reserved memory:   %llu bytes\n", pmm::hardware_reserved_ram);
        return;
    }

    // If the user inputed the help flag
    if(params.at(0).equals("-h")) {
        // Printing every available version of getmeminfo
        kprintf("-mmap - Prints the memory map given from GRUB\n");
        kprintf("-reg - Prints the blocks in the usable memory regions\n");
        return;
    }

    // Prints the memory map
    if(params.at(0).equals("-mmap")) {
        pmm::print_memory_map();
        return;
    }

    // Prints block info
    if(params.at(0).equals("-reg")) {
        pmm::print_usable_regions();
        return;
    }

    // If an invalid flag has been entered we'll throw an error
    kprintf(LOG_WARNING, "Invalid flag \"%s\"for \"getmeminfo\"!\n", params.at(0));
}

#pragma endregion
