// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// vmm.cpp
// Is the Virtual Memory Manager, takes care of paging / pages
// ========================================

#include <mm/vmm.hpp>
#include <mm/pmm.hpp>
#include <interrupts/idt.hpp>
#include <interrupts/kernel_panic.hpp>
#include <graphics/vga_print.hpp>
#include <drivers/vga.hpp>

// Getting kernels physical base from linker
extern "C" uint32_t __kernel_phys_base;

namespace vmm {
    alignas(PAGE_SIZE) pd_t* active_pd = nullptr; // The PD we'll be using
    bool enabled_paging = false;
    bool pae_paging = false;

    // Enables mapping before paging is enabled
    bool legacy_map = false;
    // Initializes the VMM with 32-bit paging
    void init(void) {
        // Allocating memory for PD
        active_pd = (pd_t*)pmm::alloc_frame(2);

        legacy_map = true;
        // Identity mapping kernel + heap + first metadata block
        identity_map_region(0x0, METADATA_ADDR + PAGE_SIZE, PRESENT | WRITABLE);
        // Mapping PMM's low_alloc_mem_head
        vmm::alloc_page((uint32_t)pmm::low_alloc_mem_head, (uint32_t)pmm::low_alloc_mem_head, PRESENT | WRITABLE);
        if(vga::framebuffer) {
            size_t pages = (vga::fb_size + PAGE_SIZE - 1) / PAGE_SIZE; // Pages needed to map VGA framebuffer
            for (size_t i = 0; i < pages; i++) {
                vmm::alloc_page((uint32_t)vga::framebuffer + i * PAGE_SIZE,
                                (uint32_t)vga::framebuffer + i * PAGE_SIZE,
                                PRESENT | WRITABLE);
            }
        }
        // Identity mapping for paging structures
        identity_map_region((uint32_t)active_pd, (uint32_t)active_pd + PAGE_SIZE * 5, PRESENT | WRITABLE);
        legacy_map = false;
        
        set_pd((uint32_t)active_pd);
        enable_paging();
        reload_cr3();
        
        // Setting up HHK (Mapping kernel to 3GiB)
        // for(uint32_t addr = (uint32_t)&__kernel_phys_base, page = KERNEL_LOAD_ADDRESS; addr < (uint32_t)&__kernel_phys_base + 0x100000; addr += PAGE_SIZE, page += PAGE_SIZE)
        //     alloc_page(page, addr, PRESENT | WRITABLE);
        // jump_to_hhk();
        
        enabled_paging = true;
        if(!vmm::is_mapped((uint32_t)active_pd)) {
            kprintf(LOG_ERROR, "Failed to initializ virtual memory manager! (Page directory is not mapped)\n");
            kernel_panic("Fatal component failed to initialize!");
        }
        else kprintf(LOG_INFO, "Implemented virtual memory manager\n");
        if(!is_mapped((uint32_t)active_pd)) enabled_paging = false;
        return;
    }

    // Allocates a 4 KiB page
    void alloc_page(const uint32_t virt_addr, const uint32_t phys_addr, const uint32_t flags) {
        if(!enabled_paging && !legacy_map) return;
        if(!active_pd) kernel_panic("PD inactive!");

        // Getting indexes from address
        uint16_t pd_index = PD_INDEX(virt_addr);
        uint16_t pt_index = PT_INDEX(virt_addr);

        // Decoding and getting flags
        bool present = flags & PRESENT;
        bool writable = flags & WRITABLE;
        bool user = flags & USER;
        bool write_through = flags & WRITETHROUGH;
        bool cache_disable = flags & NOTCACHABLE;
        bool pat = flags & PAT;
        bool global = flags & CPU_GLOBAL;

        // If PT is inactive we'll allocate it
        if(!active_pd->page_tables[pd_index]) {
            // Creating and setting flags for a new PT
            pt_t* pt = (pt_t*)pmm::alloc_frame(1);
            pd_ent pd_entry = {0};
            pd_entry.present = present;
            pd_entry.read_write = writable;
            pd_entry.user_supervisor = user;
            pd_entry.write_through = write_through;
            pd_entry.cache_disable = cache_disable;
            pd_entry.address = PHYS_TO_FRAME((uint32_t)pt);
            pd_entry.ps = 0;

            // Adding this new PT to the PD
            active_pd->page_tables[pd_index] = pt;
            active_pd->entries[pd_index] = pd_entry;
        }
        pt_t* pt = active_pd->page_tables[pd_index];

        // Creating new 4KiB page
        page_4kb new_page = {0};
        new_page.present = present;
        new_page.read_write = writable;
        new_page.user_supervisor = user;
        new_page.write_through = write_through;
        new_page.cache_disable = cache_disable;
        new_page.pat = pat;
        new_page.global = global;
        new_page.address = PHYS_TO_FRAME((uint32_t)phys_addr);

        // Setting this new page in the PT
        pt->pages[pt_index] = new_page;

        // Flush TLB
        invlpg(virt_addr);
        return;
    }

    // Allocates a 4 MiB page
    void alloc_page_4mib(const uint32_t virt_addr, const uint32_t phys_addr, const uint32_t flags) {
        if(!enabled_paging && !legacy_map) return;
        if(!active_pd) kernel_panic("PD inactive!");

        // Getting indexes from address
        uint16_t pd_index = PD_INDEX(virt_addr);

        // Decoding and getting flags
        bool present = flags & PRESENT;
        bool writable = flags & WRITABLE;
        bool user = flags & USER;
        bool write_through = flags & WRITETHROUGH;
        bool cache_disable = flags & NOTCACHABLE;

        // Creating new 4MiB page and setting flags
        pd_ent new_page = {0};
        new_page.present = present;
        new_page.read_write = writable;
        new_page.user_supervisor = user;
        new_page.write_through = write_through;
        new_page.cache_disable = cache_disable;
        new_page.ps = 1;
        new_page.address = PHYS_TO_FRAME4MB((uint32_t)phys_addr);

        active_pd->entries[pd_index] = new_page;
        // Flush TLB
        invlpg(virt_addr);
        return;
    }

    // Identity maps a region in a given range
    void identity_map_region(const uint32_t start_addr, const uint32_t end_addr, const uint32_t flags) {
        for(uint32_t addr = start_addr; addr <= end_addr; addr += PAGE_SIZE) vmm::alloc_page(addr, addr, flags);
    }

    // Frees a page at a give virtual address
    bool free_page(const uint32_t virt_addr) {
        if(!enabled_paging && !legacy_map) return true;
        if(!active_pd) kernel_panic("PD inactive!");

        // Getting indexes from address
        uint16_t pd_index = PD_INDEX(virt_addr);
        uint16_t pt_index = PT_INDEX(virt_addr);

        // If PT is inactive
        if(!active_pd->page_tables[pd_index]) return false;

        // If we're dealing with a 4MiB page
        if(active_pd->entries[pd_index].ps) {
            active_pd->entries[pd_index] = {0};
            // Flushing TLB
            invlpg(virt_addr);
            return true;
        }
        
        pt_t* pt = active_pd->page_tables[pd_index];
        // If page is inactive
        if(!pt->pages[pt_index].present) return false;

        // Freeing
        pt->pages[pt_index].present = 0;
        // Flush TLB
        invlpg(virt_addr);
        return true;
    }

    // Returns the corresponding physical address for a virtual address
    void* virtual_to_physical(const uint32_t virt_addr) {
        if(!enabled_paging && !legacy_map) return (void*)virt_addr;
        if(!active_pd) kernel_panic("PD inactive!");

        // Getting indexes from address
        uint16_t pd_index = PD_INDEX(virt_addr);
        uint16_t pt_index = PT_INDEX(virt_addr);

        // If the PDE is a 4MiB page
        if(active_pd->entries[pd_index].present && active_pd->entries[pd_index].ps)
            return (void*)FRAME4MB_TO_PHYS(active_pd->entries[pd_index].address);

        // If PT is inactive
        if(!active_pd->page_tables[pd_index]) return nullptr;
        pt_t* pt = active_pd->page_tables[pd_index];
        // If page is inactive
        if(!pt->pages[pt_index].present) return nullptr;

        // returning the physical address gotten from the address field in the 4KiB page
        return (void*)FRAME_TO_PHYS(pt->pages[pt_index].address) + PAGE_OFFSET(virt_addr);
    }

    // Returns if a page at the given virtual address is mapped or not
    bool is_mapped(const uint32_t virt_addr) {
        if(!enabled_paging && !legacy_map) return false;
        if(!active_pd) kernel_panic("PD inactive!");

        // Getting indexes from address
        uint16_t pd_index = PD_INDEX(virt_addr);
        uint16_t pt_index = PT_INDEX(virt_addr);

        // If PT is inactive
        if(active_pd->entries[pd_index].present == 0) return false;
        // If page is inactive
        if(active_pd->page_tables[pd_index]->pages[pt_index].present == 0) return false;

        // If we made it to here it means the page is mapped
        return true;
    }
}
