# Memory Managers Overview

This file will document the heap manager, virtual memory manager and the physical memory manager.

## Heap memory

Kernel heap ranges from the kernels physical base + 0x100000 in memory and takes up 3 MiB.
The heap manager is based on the kmalloc function (defined in src/kernel/mm/heap.cpp) that  uses a first fit allocation system and a linked list to dynamically allocate different size blocks. These functions also merge/split blocks to make sure as less amount of usable memory goes to waste.
At this point the kernel heap has two allocation and one deallocation function: 

### Allocating Functions in the Heap 
1. kmalloc
    Allocates a block in the kernel heap with a certain size given by parameter "size".

2. kcalloc
    Allocates a "num" number of "size" sized blocks to the kernel heap for an array.

### Deallocating Functions in the Heap
1. kfree
    Retreives and deallocates a block for a given address "ptr".


## Physical Memory Manager

The physical memory manager in MioOS retreives a memory map wich is given by GRUB, it manages this info and creates a linked list to efficiently use the found usable memory regions.
The lower usable memory region usually starts at the kernels physical base, while the higher region usually starts at the 4GiB mark. alloc_frame uses a first fit allocation method and also merges/splits neighbor blocks for minimal waisted memory like the heap manager. At this point there is two allocating and one deallocating function in the PMM:

### Allocating Functions in the PMM
1. alloc_frame
    Allocates a frame in RAM with a size of give number of 4KiB blocks

2. alloc_frame_aligned
    Allocates the same way as alloc_frame but it alignes the value to 4KiB. This is only used for allocations envolving paging structures
