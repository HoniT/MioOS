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
#include <drivers/vga_print.hpp>
#include <lib/math.hpp>
#include <lib/mem_util.hpp>

// Getting the kernels physical starting address from the linker script
extern "C" uint32_t __kernel_phys_base;

// Amount of certain types of memory
uint64_t pmm::total_usable_ram = 0;
uint64_t pmm::total_used_ram = 0;
uint64_t pmm::hardware_reserved_ram = 0;
// Multiboot info will be saved here
multiboot_info* mb_info = nullptr;

uint64_t LOW_DATA_START_ADDR = 0;
uint64_t metadata_reserved = 0; // Space reserved by frame metadata

// We will have two different heads for two allocatable regions 
MetadataNode* pmm::low_alloc_mem_head = nullptr; // Lower allocatable RAM: usually 0x100000 - 0xBFFDFFFF
MetadataNode* pmm::high_alloc_mem_head = (MetadataNode*)(METADATA_ADDR + sizeof(MetadataNode)); // High allocatable RAM: usually 4 GiB and onwards

#pragma region Memory Map Manager

// Prints out the memory map: debugging / helper function
void pmm::print_memory_map(void) {
    if (!mb_info) {
        // Memory map not defined
        return;
    }

    // Getting memory map length and address
    uint32_t mmap_length = mb_info->mmap_length;
    uint32_t mmap_addr = mb_info->mmap_addr;

    // Iterate over the memory map entries
    struct multiboot_mmap_entry* mmap = (struct multiboot_mmap_entry*)mmap_addr;
    while (uint32_t(mmap) < mmap_addr + mmap_length) {
        // Calculate the end address
        uint64_t end_addr = mmap->addr + mmap->len - 1;

        // Print memory region information
        vga::printf("Memory region: %lx - %lx, Type: %s\n",
               mmap->addr, end_addr,
               mmap->type == 1 ? "Available" : "Reserved");

        // Move to the next entry
        mmap = (struct multiboot_mmap_entry*)(uint32_t(mmap) + mmap->size + sizeof(mmap->size));
    }
}

// Prints info of blocks in the allocatable memory regions 
void pmm::print_usable_regions(void) {
    // Getting list head
    MetadataNode* current = low_alloc_mem_head;

    // Itterating through blocks
    while(current) {
        // Printing info
        vga::printf("-Block range: %lx-%lx\n", current->addr, current->addr + current->size - 1);
        vga::printf("Block size: %lx\n", current->size);
        vga::printf("Blocks of memory taken up: %d\n", udiv64(current->size, FRAME_SIZE));
        vga::printf("Block status: %s\n", (current->free) ? "Free" : "Allocated");

        // Going to next block
        current = current->next;
    }
}

// Getts the total amount of usable RAM in the system and creates head (and other) nodes for memory alloc
void pmm::manage_mmap(struct multiboot_info* _mb_info) {
    if (!(_mb_info->flags & (1 << 6))) {
        // Memory map not available
        vga::error("Multiboot info not available! Address: %x\n", _mb_info);
        return;
    }

    // Getting memory map length and address
    uint32_t mmap_length = _mb_info->mmap_length;
    uint32_t mmap_addr = _mb_info->mmap_addr;

    // Itterating through mmap entries info
    struct multiboot_mmap_entry* mmap = (struct multiboot_mmap_entry*)mmap_addr;
    while (uint32_t(mmap) < mmap_addr + mmap_length) {
        // If the mmap entry is available
        if(mmap->type == 1) {
            // Keeping the total RAM amount for stats
            pmm::total_usable_ram += mmap->len;

            // If the mmap entry is at 4GiB+
            if(mmap->addr >= 0x100000000) {
                // Manage high allocatable ram
                high_alloc_mem_head = (MetadataNode*)(METADATA_ADDR + sizeof(MetadataNode)); // The second node of the list
                high_alloc_mem_head->addr = 0x100000000; // 4GiB (High usable region start)
                high_alloc_mem_head->size = mmap->len;
                high_alloc_mem_head->free = true; // Noting that this is free
                high_alloc_mem_head->next = nullptr; // Making this the linked list tail
                high_alloc_mem_head->prev = low_alloc_mem_head; // Making the low allocatable region the previous
            }
            // If the mmap entry address is at the kernels base
            else if(mmap->addr == uint64_t(&__kernel_phys_base)) {
                // Creting node for low allocatable ram 
                low_alloc_mem_head = (MetadataNode*)METADATA_ADDR;
                low_alloc_mem_head->addr = 0; // We will set this later
                low_alloc_mem_head->size = mmap->addr + mmap->len; // We will change this later
                low_alloc_mem_head->free = true; // Noting that this is free
                low_alloc_mem_head->next = high_alloc_mem_head; // Making the high usable region the next node
                low_alloc_mem_head->prev = nullptr; // Making this the linked list head
            }
            /* There is one available mmap entry that starts at 0x0, but we will not use this because
             * to not conflict with any BIOS info/memory */
            else pmm::hardware_reserved_ram += mmap->len;
        }
        // If the entry is reserved we'll add the size to the total amount of reserved ram
        else pmm::hardware_reserved_ram += mmap->len;

        // Move to the next entry
        mmap = (struct multiboot_mmap_entry*)(uint32_t(mmap) + mmap->size + sizeof(mmap->size));
    }

    // Calculating amount of space to reserve for metadata
    metadata_reserved = pmm::total_usable_ram / FRAME_SIZE * sizeof(MetadataNode);
    // Keeping the address in where the actual data will be placed and aligning it to ensore page-alignment
    LOW_DATA_START_ADDR = align_up(METADATA_ADDR + metadata_reserved, PAGE_SIZE);

    // Printing the total amount of RAM
    vga::printf("Total amount of usable RAM: ~%u\n", uint32_t(pmm::total_usable_ram / BYTES_IN_GIB));
}

#pragma endregion

// Initializes any additional info for the PMM
void pmm::init(struct multiboot_info* _mb_info) {
    // Saving multiboot info globally
    mb_info = _mb_info;
    // Calling needed functions
    pmm::manage_mmap(_mb_info);

    // Setting up low_alloc_mem_head
    low_alloc_mem_head->addr = LOW_DATA_START_ADDR;
    low_alloc_mem_head->size -= LOW_DATA_START_ADDR; /* We previosly set this as the mmap entry's address + size, 
                                                      * now we'll just subtract the actual data starting address to just get the size */

    vga::printf("PMM initialized!\n");
}

// Alloc and dealloc

// Allocates a frame in the usable memory regions
void* pmm::alloc_frame(const uint64_t num_blocks) {
    // Precausions
    if(num_blocks <= 0) return nullptr;

    // Aligning to default frame size
    uint64_t size = num_blocks * FRAME_SIZE;

    MetadataNode* current = low_alloc_mem_head; // Setting the current block as the head

    #ifdef VMM_HPP
    // Checks if the list head is mapped
    if (enabled_paging && !vmm::is_mapped(uint64_t(low_alloc_mem_head))) {
        vga::error("Page fault: heap_head is not mapped!\n");
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
                MetadataNode* new_block = (MetadataNode*)(current + sizeof(MetadataNode));
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
            if(enabled_paging) {
                // Identity map the allocated frames every 4KiB block to virtual memory
                for(uint64_t block = 0, addr = return_address; block < num_blocks; block++, addr += PAGE_SIZE)
                    vmm::map_page(addr, addr, PRESENT | WRITABLE);
            }
            #endif // VMM_HPP

            return (void*)return_address;
        }
        // Moving to the next block
        current = current->next;
    }

    vga::error("Not enough memory to allocate %x block(s)!\n", num_blocks);
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
        if(enabled_paging) {
            // Unmap the allocated frames every 4KiB block from virtual memory
            for(uint64_t i = 0, addr = uint64_t(ptr); i < (block->size / PAGE_SIZE); block++, addr += PAGE_SIZE)
                vmm::unmap_page(uint64_t(addr));
        }
    #endif // VMM_HPP
}
