// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// pmm.cpp
// Fixed: Address recalculation, Metadata collisions, and Reservation size
// ========================================

#include <mm/pmm.hpp>
#include <mm/heap.hpp>
#include <mm/vmm.hpp>
#include <multiboot.hpp>
#include <graphics/vga_print.hpp>
#include <x86/interrupts/kernel_panic.hpp>
#include <lib/math.hpp>
#include <lib/mem_util.hpp>
#include <lib/string_util.hpp>

// Getting the kernels physical starting address from the linker script
extern "C" uint32_t __kernel_phys_base;
uint32_t pmm::get_kernel_addr() {return uint32_t(&__kernel_phys_base);}
extern "C" uint32_t __kernel_end;
uint32_t pmm::get_kernel_end() {return uint32_t(&__kernel_end);}
extern "C" uint32_t __kernel_size;
uint32_t pmm::get_kernel_size() {return uint32_t(&__kernel_size);}

// Amount of certain types of memory
uint64_t pmm::total_usable_ram = 0;
uint64_t pmm::total_used_ram = 0;
uint64_t pmm::hardware_reserved_ram = 0;
uint64_t pmm::total_installed_ram = 0;

// Multiboot info will be saved here
void* mb2_info = nullptr;

uint64_t LOW_DATA_START_ADDR = 0;
uint64_t metadata_reserved = 0; // Space reserved by frame metadata

// Allocator for the metadata nodes themselves
static MetadataNode* metadata_pool_start = nullptr;
static MetadataNode* metadata_pool_cursor = nullptr;
static uint64_t metadata_pool_end = 0;

// We will have two different heads for two allocatable regions 
MetadataNode* pmm::low_alloc_mem_head = nullptr; 
MetadataNode* pmm::high_alloc_mem_head = nullptr;

// Helper to get a fresh metadata node from the reserved region
MetadataNode* get_new_metadata_node() {
    if ((uint64_t)metadata_pool_cursor >= metadata_pool_end) {
        kprintf(LOG_ERROR, "PMM: Out of reserved metadata space!\n");
        return nullptr;
    }
    MetadataNode* node = metadata_pool_cursor;
    metadata_pool_cursor++; // Bump pointer
    memset(node, 0, sizeof(MetadataNode));
    return node;
}

#pragma region Memory Map Manager

// Prints out the memory map: debugging / helper function
void pmm::print_memory_map(void) {
    if (!mb2_info) {
        kprintf(LOG_ERROR, "Memory map not defined\n");
        return;
    }

    multiboot_tag_mmap* mmap_tag = Multiboot2::get_mmap(mb2_info);
    if (!mmap_tag) {
        kprintf(LOG_ERROR, "Memory map tag not found\n");
        return;
    }

    uint32_t entry_count = (mmap_tag->size - sizeof(multiboot_tag_mmap)) / mmap_tag->entry_size;
    
    for (uint32_t i = 0; i < entry_count; i++) {
        multiboot_mmap_entry* entry = &mmap_tag->entries[i];
        uint64_t end_addr = entry->addr + entry->len - 1;
        const char* type_str;
        
        switch(entry->type) {
            case MULTIBOOT_MEMORY_AVAILABLE: type_str = "Available"; break;
            case MULTIBOOT_MEMORY_RESERVED:  type_str = "Reserved"; break;
            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE: type_str = "ACPI Reclaimable"; break;
            case MULTIBOOT_MEMORY_NVS:       type_str = "ACPI NVS"; break;
            case MULTIBOOT_MEMORY_BADRAM:    type_str = "Bad RAM"; break;
            default:                         type_str = "Unknown"; break;
        }

        kprintf("Memory region: %llx - %llx, Type: %s\n", entry->addr, end_addr, type_str);
    }
}

// Gets the total amount of usable RAM in the system and creates head (and other) nodes for memory alloc
void pmm::manage_mmap(void* _mb2_info) {
    multiboot_tag_mmap* mmap_tag = Multiboot2::get_mmap(_mb2_info);
    if (!mmap_tag) {
        kprintf(LOG_ERROR, "Memory map not available in Multiboot2 info!\n");
        return;
    }

    uint32_t entry_count = (mmap_tag->size - sizeof(multiboot_tag_mmap)) / mmap_tag->entry_size;

    // 1. First Pass: Calculate total RAM to reserve Metadata space correctly
    for (uint32_t i = 0; i < entry_count; i++) {
        multiboot_mmap_entry* entry = &mmap_tag->entries[i];
        if(entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
             if(entry->addr >= 0x100000000 && vmm::pae_paging) {
                pmm::total_usable_ram += entry->len;
             } else if(entry->addr == uint64_t(&__kernel_phys_base)) {
                pmm::total_usable_ram += entry->len;
             }
        } else {
            pmm::hardware_reserved_ram += entry->len;
        }
        pmm::total_installed_ram += entry->len;
    }

    // We need 1 node per frame roughly in worst case fragmentation.
    uint64_t total_frames = pmm::total_usable_ram / FRAME_SIZE;
    metadata_reserved = total_frames * sizeof(MetadataNode);
    
    // Add a safety buffer (e.g., 256 extra nodes)
    metadata_reserved += 256 * sizeof(MetadataNode);

    // Initialize the Metadata Pool pointers
    metadata_pool_start = (MetadataNode*)METADATA_ADDR;
    metadata_pool_cursor = metadata_pool_start;
    metadata_pool_end = METADATA_ADDR + metadata_reserved;
    
    // The actual data (physical frames) starts AFTER the reserved metadata region
    LOW_DATA_START_ADDR = align_up(METADATA_ADDR + metadata_reserved, PAGE_SIZE);

    // 2. Second Pass: Initialize the Head Nodes using the pool
    for (uint32_t i = 0; i < entry_count; i++) {
        multiboot_mmap_entry* entry = &mmap_tag->entries[i];

        if(entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            
            // High Allocatable RAM (4GiB+)
            if(entry->addr >= 0x100000000 && vmm::pae_paging) {
                // Allocate node from our pool, NOT via blind pointer arithmetic
                high_alloc_mem_head = get_new_metadata_node();
                
                high_alloc_mem_head->addr = entry->addr; // 0x100000000 usually
                high_alloc_mem_head->size = entry->len;
                high_alloc_mem_head->free = true;
                high_alloc_mem_head->next = nullptr;
                // Prev is set later or null if it's the absolute head, 
                // but usually we link low -> high.
            }
            // Low Allocatable RAM (Kernel Base -> 3GB/End)
            else if(entry->addr == uint64_t(&__kernel_phys_base)) {
                low_alloc_mem_head = get_new_metadata_node();
                
                low_alloc_mem_head->addr = LOW_DATA_START_ADDR;
                // Calculate size: End of region - Start of Data
                // (entry->addr + entry->len) is the end of the BIOS map region
                low_alloc_mem_head->size = (entry->addr + entry->len) - LOW_DATA_START_ADDR; 
                low_alloc_mem_head->free = true;
                low_alloc_mem_head->prev = nullptr;
            }
        }
    }

    // Link the regions if both exist
    if (low_alloc_mem_head) {
        if (vmm::pae_paging && high_alloc_mem_head) {
            low_alloc_mem_head->next = high_alloc_mem_head;
            high_alloc_mem_head->prev = low_alloc_mem_head;
        } else {
            low_alloc_mem_head->next = nullptr;
        }
    }
}

#pragma endregion

// Initializes any additional info for the PMM
void pmm::init(void* _mb2_info) {
    mb2_info = _mb2_info;
    pmm::manage_mmap(_mb2_info);
    
    if(!low_alloc_mem_head) {
        kprintf(LOG_ERROR, "Failed to initialize PMM! (Low allocable memory not defined)\n");
        kernel_panic("Fatal component failed to initialize!");
    }
   
   // Clean up any unwanted memory from a warm boot in the first free block
   memset((void*)low_alloc_mem_head->addr, 0, low_alloc_mem_head->size);
   
   kprintf(LOG_INFO, "Implemented physical memory manager\n");
}

// Allocates a frame in the usable memory regions
void* pmm::alloc_frame(const uint64_t num_blocks, bool identity_map) {
    if(num_blocks <= 0) return nullptr;

    uint64_t size = num_blocks * FRAME_SIZE;
    MetadataNode* current = low_alloc_mem_head;

    #ifdef VMM_HPP
    if (vmm::enabled_paging && !vmm::is_mapped(uint32_t(low_alloc_mem_head))) {
        kprintf(LOG_ERROR, "Page fault: pmm_head is not mapped!\n");
        return nullptr;
    }
    #endif

    while(current) {
        if(current->free && current->size >= size) {
            // Split the block if there is extra space
            if(current->size > size) {
                MetadataNode* new_block = get_new_metadata_node();
                if (!new_block) return nullptr; // OOM in metadata pool

                // Setup the split (free) block
                new_block->size = current->size - size;
                // FIX: Calculate absolute address based on current, NOT current->prev
                new_block->addr = current->addr + size; 
                new_block->free = true;
                
                // Link it in
                new_block->next = current->next;
                new_block->prev = current;
                
                if (current->next) {
                    current->next->prev = new_block;
                }
                current->next = new_block;
            }

            // Update current block (Allocated)
            current->size = size;
            current->free = false; 
            
            pmm::total_used_ram += size;

            uint64_t return_address = current->addr;
            
            #ifdef VMM_HPP 
            if(vmm::enabled_paging && identity_map) {
                vmm::identity_map_region(return_address, return_address + PAGE_SIZE * num_blocks, PRESENT | WRITABLE);
            }
            #endif 
            
            memset((void*)return_address, 0, FRAME_SIZE * num_blocks);
            return (void*)align_up(return_address, PAGE_SIZE); 
        }
        current = current->next;
    }

    kprintf(LOG_ERROR, "Not enough memory to allocate %x block(s)!\n", num_blocks);
    return nullptr;
}

void* pmm::alloc_frame_by_size(uint64_t size, bool identity_map) {
    uint64_t num_blocks = size / FRAME_SIZE;
    if (size % FRAME_SIZE != 0) num_blocks++;
    return pmm::alloc_frame(num_blocks, identity_map);
}

// Frees a frame
void pmm::free_frame(void* ptr) {
    if(!ptr) return;

    MetadataNode* current = low_alloc_mem_head;
    MetadataNode* block = nullptr;
    
    // Find the block
    while(current) {
        if(current->addr == uint64_t(ptr)) {
            block = current;
            break;
        }
        current = current->next;
    }
    
    if (!block) return; // Block not found

    block->free = true;
    pmm::total_used_ram -= block->size;

    // Merge pass
    // We restart from head to ensure we catch all merge opportunities or reverse merge
    bool merged_frames = true;
    
    while (merged_frames) {
        merged_frames = false;
        current = low_alloc_mem_head;
        
        while(current) {
            if(current->free && current->next && current->next->free) {
                // Crucial Check: Contiguity
                // Only merge if they are physically adjacent
                if(current->next->addr == current->addr + current->size) {
                    
                    current->size += current->next->size;
                    
                    // Unlink the next node
                    // Note: In a bump allocator metadata system, the unlinked node 
                    // is "leaked" inside the metadata pool. This is acceptable for 
                    // simple PMMs, or you can implement a free-list for metadata nodes.
                    MetadataNode* node_to_remove = current->next;
                    
                    current->next = node_to_remove->next;
                    if(current->next)
                        current->next->prev = current;
                    
                    merged_frames = true;
                }
            }
            current = current->next;
        }
    }

    #ifdef VMM_HPP
        if(vmm::enabled_paging) {
            for(uint64_t i = 0; i < (block->size / PAGE_SIZE); i++)
                vmm::free_page(uint64_t(ptr) + i * PAGE_SIZE);
        }
    #endif
}