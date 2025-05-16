# Memory Managers Overview

This file will document the heap manager, virtual memory manager and the physical memory manager.

## Heap memory

Kernel heap ranges from the kernels physical base + 0x100000 in memory and takes up 3 MiB.
The heap manager is based on the kmalloc function (defined in src/kernel/mm/heap.cpp) that  uses a first fit allocation system and a linked list to dynamically allocate different size blocks. These functions also merge/split blocks to make sure as less amount of usable memory goes to waste.
At this point the kernel heap has two allocation and one deallocation function: 

### Allocating Functions in the Heap 
1. kmalloc(size_t size)
    Allocates a block in the kernel heap with a certain size given by parameter "size".

2. kcalloc(size_t num, size_t size)
    Allocates a "num" number of "size" sized blocks to the kernel heap for an array.

### Deallocating Functions in the Heap
1. kfree(void* ptr)
    Retreives and deallocates a block for a given address "ptr".


## Physical Memory Manager

The physical memory manager in MioOS retreives a memory map wich is given by GRUB, it manages this info and creates a linked list to efficiently use the found usable memory regions.
The lower usable memory region usually starts at the kernels physical base, while the higher region usually starts at the 4GiB mark. alloc_frame uses a first fit allocation method and also merges/splits neighbor blocks for minimal waisted memory like the heap manager. At this point there is one allocating and one deallocating function in the PMM:

### Allocating Functions in the PMM
1. pmm::alloc_frame(const uint64_t num_blocks)
    Allocates a frame in RAM with a size of a give number of "num_blocks" 4KiB blocks

### Deallocatin Functions in thr PMM
1. pmm::free_frame(void* ptr)
    Frees a frame with the given address of parameter "ptr"

## Virtual Memory Manager

The virtual memory manager takes care of setting up paging, mapping and unmaping pages. MioOS uses 32-bit paging that supports up to 4 GiB of RAM, which is more than enough for this system. The VMM identity maps needed regions in the initializing function, has alloc_page, alloc_page_4mib and free_page to map addresses to virtual memory with given flags. The VMM also supports debugging/helper functions like to translate a virtual address to a physical one, or to check if a page at a given virtual address is mapped.

### Mapping Functions in the VMM
1. vmm::alloc_page(const uint32_t virt_addr, const uint32_t phys_addr, const uint32_t flags)
    This maps a given physical address to a given virtual address with a given set of flags

2. vmm::alloc_page_4mib(const uint32_t virt_addr, const uint32_t phys_addr, const uint32_t flags)
    This maps a given physical address to a given virtual address in a 4MiB page with a given set of flags

### Unmapping Functions in the VMM
1. vmm::free_page(const uint32_t virt_addr)
    This unmaps the page at a given virtual address "virt_addr"
    