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

// Amount of usable memory
uint64_t pmm::total_usable_ram = 0;
uint64_t pmm::total_used_ram = 0;
// Multiboot info will be saved here
multiboot_info* mb_info = nullptr;

// We will have two different heads for two allocatable regions 
MemoryNode* pmm::low_alloc_mem_head = nullptr; // Lower allocatable RAM: usually 0x100000 - 0xBFFDFFFF
MemoryNode* pmm::high_alloc_mem_head = nullptr; // High allocatable RAM: usually 4 GiB and onwards

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
    MemoryNode* current = low_alloc_mem_head;

    // Itterating through blocks
    while(current) {
        // Printing info
        vga::printf("-Block range: %lx-%lx\n", uint64_t(current), uint64_t(uint64_t(current) + current->size));
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

            // If the mmap entry address is at 4GiB+
            if(mmap->addr >= 0x100000000) {
                // Creting node for high allocatable ram 
                // high_alloc_mem_head = (MemoryNode*)mmap->addr;
                // high_alloc_mem_head->size = mmap->len - sizeof(MemoryNode); // Making this nodes size the length of the region and minus the metadata size
                // high_alloc_mem_head->free = true; // Noting that this node is free
                // high_alloc_mem_head->next = nullptr; // Noting that this is the tail of the linked list
            } else if(mmap->addr >= 0x100000) { // If the mmap entry is at 1MiB+
                // Creting node for low allocatable ram 
                low_alloc_mem_head = (MemoryNode*)DATA_LOW_START_ADDR;
                low_alloc_mem_head->size = mmap->len - (DATA_LOW_START_ADDR - mmap->addr) - 1; // The first node of the low allocatable memory is the kernel region 0x100000-0x200000
                low_alloc_mem_head->free = true; // Noting that this is occupied
                low_alloc_mem_head->next = nullptr; // Making the next node the heap node
            }
        }

        // Move to the next entry
        mmap = (struct multiboot_mmap_entry*)(uint32_t(mmap) + mmap->size + sizeof(mmap->size));
    }
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

    vga::printf("PMM initialized!\n");
}

// Alloc and dealloc

// Allocates a frame in the usable memory regions
void* pmm::alloc_frame(const uint64_t num_blocks) {
    // Precausions
    if(num_blocks <= 0) return nullptr;

    // Aligning to default frame size
    uint64_t size = num_blocks * FRAME_SIZE;

    MemoryNode* current = low_alloc_mem_head; // Setting the current block as the head

    #ifdef VMM_HPP
    // if (enabled_paging && !vmm::is_mapped((void*)(low_alloc_mem_head))) {
    //     vga::error("Page fault: heap_head is not mapped!\n");
    //     return nullptr;
    // }
    #endif

    // Find a free block with enough space
    while(current) {
        // If the current block is free and has enough space then we can use it
        if(current->free && current->size >= size) {
            // Split the block if theres extra space
            if(current->size > size + sizeof(MemoryNode)) {
                // Setting new block and adding the sizeof the Block to keep metadata in mind
                MemoryNode* new_block = (MemoryNode*)((char*)current + sizeof(MemoryNode) + size);
                // Trimming the extra space of
                new_block->size = current->size - size - sizeof(MemoryNode);
                new_block->free = true;
                /* Setting the current blocks next block as the trimmed (new)
                 * blocks next, because the new block is the end of the current block */
                new_block->next = current->next;
                // Making the current blocks next block the new block
                current->next = new_block;
            }

            current->size = size; // Setting the passed size
            current->free = false; // Mark as allocated

            // Noting that we took up usable RAM
            pmm::total_used_ram += size;

            // Returning the current blocks address plus the metadata size needed and aligning it to the page size
            void* return_address = (char*)current + sizeof(MemoryNode);

            #ifdef VMM_HPP // If VMM is present
            // If paging is enabled
            if(enabled_paging) {
                // Map the allocated frame to virtual memory
            }
            #endif // VMM_HPP

            return return_address;
        }
        // Moving to the next block
        current = current->next;
    }

    vga::error("Not enough memory to allocate %x block(s)!\n", num_blocks);
    return nullptr;
}

/* Allocates blocks and returns the address aligned to page size.
 * This should only be use for critical allocations that absolutely need pagesize-aligned addresses,
 * because this waists PAGE_SIZE - sizeof(MemoryNode) of usable space just to be aligned */
void* pmm::alloc_frame_aligned(const uint64_t num_blocks) {
    // We allocate num_blocks + 1, the +1 is just there to insure alignment
    return (void*)align_up(size_t(pmm::alloc_frame(num_blocks + 1)), PAGE_SIZE);
}

// Frees a frame
void pmm::free_frame(const void* ptr) {
    if(!ptr) return;

    // Getting the frame based of of the given address/pointer and setting it as free
    MemoryNode* block = (MemoryNode*)((char*)ptr - sizeof(MemoryNode));
    block->free = true; // Setting as free
    // Noting that we freed a block
    pmm::total_used_ram -= block->size;

    // Setting the corresponding linked list head, high or low usable mem region
    MemoryNode* current = low_alloc_mem_head;

    // Merging adjacent free blocks to decrease fragmentation
    while(current) {
        // If the current block and the next block are free
        if(current->free && current->next && current->next->free) {
            // Adding the block sizes together
            current->size += current->next->size + sizeof(HeapBlock);
            /* The current blocks next block will be the next blocks next block, 
            * because the next block doesn't exist anymore */
            current->next = current->next->next;

            // Freeing the memory of the deleted node
            free_frame(current->next);
        }
        // Going to the next block
        current = current->next;
    }

    // Setting the ptr to null because it doesn't point to a valid address anymore
    ptr = nullptr;
}
