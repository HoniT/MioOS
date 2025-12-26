# Memory Management Subsystem

This document provides a technical overview of the **MioOS** memory management architecture, covering the Physical Memory Manager (PMM), Virtual Memory Manager (VMM), and the Kernel Heap.

## Architecture Overview

The memory management subsystem consists of three distinct layers that interact to provide memory services to the kernel:

1.  **PMM (Physical Memory Manager):** The bottom layer. It talks directly to the hardware and tracks which physical RAM frames are free or used.
2.  **VMM (Virtual Memory Manager):** The middle layer. It translates virtual addresses used by software into physical addresses managed by the PMM using paging.
3.  **Heap Manager:** The top layer. It provides standard `malloc`/`free` style allocation for the kernel by managing a specific region of virtual memory.

---

## 1. Physical Memory Manager (PMM)

The PMM is responsible for tracking the state of physical RAM. It retrieves the memory map provided by the GRUB bootloader to identify usable memory regions (filtering out reserved hardware areas).

### Internal Logic
Unlike simple bitmap allocators, the MioOS PMM uses a **Linked List Free-Store** algorithm.
* **Initialization:** It parses the GRUB memory map to build the initial list of free regions.
* **Allocation Strategy:** It utilizes a **First-Fit** algorithm to find a continuous block of physical frames.
* **Optimization:** The manager actively merges adjacent free blocks and splits larger blocks when necessary to minimize external fragmentation and wasted memory.

### Physical Memory Regions
* **Low Memory:** Starts at the kernel's physical base.
* **High Memory:** Typically starts at the 4 GiB mark (depending on hardware and map).

### PMM API Reference

| Function | Signature | Description |
| :--- | :--- | :--- |
| **Allocate Frame** | `alloc_frame(num_blocks, identity_map)` | Allocates a contiguous physical area consisting of `num_blocks` (where 1 block = 4KiB).<br>**Params:**<br>`num_blocks`: Count of 4KiB frames needed.<br>`identity_map`: If `true`, immediately identity maps the region in the VMM. |
| **Free Frame** | `free_frame(ptr)` | Returns a physical frame to the free list.<br>**Params:**<br>`ptr`: The physical address to free. |

---

## 2. Virtual Memory Manager (VMM)

The VMM manages the CPU's paging structures (Page Tables and Page Directories). MioOS utilizes **x86 32-bit Paging**, supporting up to 4 GiB of addressable virtual memory.

### Features
* **Paging Mode:** Standard 32-bit paging with support for both **4KiB** and **4MiB** pages.
* **Identity Mapping:** Capabilities to map virtual addresses 1:1 to physical addresses (essential for kernel initialization and hardware drivers).
* **Helpers:** Includes utilities for Virtual-to-Physical translation and page status checking.

### VMM API Reference

| Function | Signature | Description |
| :--- | :--- | :--- |
| **Map 4KiB** | `alloc_page(virt, phys, flags)` | Maps a specific physical address to a virtual address using a standard 4KiB page.<br>**Flags:** Read/Write, User/Supervisor, Present, etc. |
| **Map 4MiB** | `alloc_page_4mib(virt, phys, flags)` | Maps a physical address to a virtual address using a large 4MiB page (reduces TLB misses for large structures). |
| **Identity Map** | `identity_map_region(start, end, flags)` | Maps a range of addresses (`start` to `end`) such that `VirtAddr == PhysAddr`. |
| **Unmap** | `free_page(virt_addr)` | Unmaps the page at the given virtual address, invalidating the entry in the page table. |

---

## 3. Kernel Heap

The Kernel Heap provides dynamic memory allocation for kernel structures (pointers, arrays, objects). It abstracts away the page-aligned nature of the PMM/VMM into byte-granular allocations.

### Heap Layout
* **Location:** `Kernel Physical Base + 0x100000`
* **Initial Size:** 3 MiB
* **Algorithm:** **First-Fit Linked List**. The heap manager iterates through a list of memory headers to find the first block large enough to satisfy a request. It supports block splitting (to use only what is needed) and block coalescing (merging adjacent free blocks upon deallocation).

### Heap API Reference

| Function | Signature | Description |
| :--- | :--- | :--- |
| **Allocate** | `kmalloc(size)` | Allocates a contiguous block of memory on the kernel heap.<br>**Params:**<br>`size`: Size in bytes. |
| **Array Alloc** | `kcalloc(num, size)` | Allocates an array of `num` elements, each of `size` bytes, and initializes the memory to zero. |
| **Free** | `kfree(ptr)` | Releases a previously allocated block back to the heap pool.<br>**Params:**<br>`ptr`: Pointer to the memory block. |
