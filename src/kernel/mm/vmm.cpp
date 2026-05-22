// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Virtual memory manager state and init
// ========================================

#include <mm/vmm.hpp>
#include <mm/paging.hpp>
#include <mm/pmm.hpp>
#include <mm/kheap.hpp>

#include <graphics/kprint.hpp>
#include <kernel_panic.hpp>

using namespace mem;

// Bootloader asm references
extern "C" PageTable p4_table;
extern "C" PageTable p3_table;

// Linker references
extern "C" uint8_t _text_start[];
extern "C" uint8_t _text_end[];
extern "C" uint8_t _rodata_start[];
extern "C" uint8_t _rodata_end[];
extern "C" uint8_t _data_start[];
extern "C" uint8_t _bss_end[];
extern "C" uint8_t kernel_end_phys[];
extern "C" uint8_t kernel_start_phys[];

constexpr const char* paging_error_to_string(PagingError err) noexcept {
    switch (err) {
        case PagingError::Success:          return "Success";
        case PagingError::OutOfMemory:      return "OutOfMemory";
        case PagingError::AlreadyMapped:    return "AlreadyMapped";
        case PagingError::NotMapped:        return "NotMapped";
        case PagingError::InvalidAlignment: return "InvalidAlignment";
        case PagingError::InvalidContext:   return "InvalidContext";
        case PagingError::InvalidAddress:   return "InvalidAddress";
        case PagingError::HardwareFault:    return "HardwareFault";
        default:                            return "Unknown";
    }
}

alignas(VMM) static uint8_t g_vmm_storage[sizeof(VMM)];
VMM* g_vmm = nullptr;

void VMM::init() noexcept {
    const PhysAddr boot_pml4_phys =
        reinterpret_cast<VirtAddr>(&p4_table) - HIGHER_HALF_OFFSET;

    g_vmm = new (g_vmm_storage) VMM(boot_pml4_phys);

    if (!g_vmm->m_backend.kernel_context().is_valid())
    kernel_panic("VMM: kernel PML4 allocation failed — PMM has no free frames\n");
    
    if (!g_vmm->m_kernel_as.context().is_valid())
    kernel_panic("VMM: kernel AddressSpace context invalid after construction\n");
    
    if (!PagingBackend::nx_supported())
    kprintf("VMM: WARNING — NX/XD bit not supported by CPU.\n");

    auto reg = [&](VirtAddr vstart, VirtAddr vend,
                    PageFlags flags, const char* name) noexcept {
        const VirtAddr base = page_align_down(vstart);
        const usize    sz   = page_align_up(vend) - base;

        PagingError err = g_vmm->m_kernel_as.register_vma(
            base, sz, flags | PageFlags::Global, VMAType::Kernel);

        if (!is_ok(err)) { 
            kprintf("VMM: VMA REGISTRATION FAILED\n");
            kprintf("Region:");
            kprintf(name);
            kprintf("\nError: ");
            kprintf(paging_error_to_string(err));
            kernel_panic("\nVMM: register_vma failed"); 
        }
    };

    reg(reinterpret_cast<VirtAddr>(_text_start),
        reinterpret_cast<VirtAddr>(_text_end),
        PageFlags::KernelRX,  ".text");

    reg(reinterpret_cast<VirtAddr>(_rodata_start),
        reinterpret_cast<VirtAddr>(_rodata_end),
        PageFlags::KernelRO,  ".rodata");

    reg(reinterpret_cast<VirtAddr>(_data_start),
        reinterpret_cast<VirtAddr>(_bss_end),
        PageFlags::KernelRW,  ".data+.bss");

    // PMM bitmap — init_pmm placed it at kernel_end_phys + HIGHER_HALF_OFFSET.
    {
        const uint64_t kernel_end_pa  = reinterpret_cast<uint64_t>(kernel_end_phys);
        const uint64_t bitmap_pa      =
            (kernel_end_pa + FRAME_SIZE - 1) & ~(FRAME_SIZE - 1);
        const VirtAddr bitmap_virt    = bitmap_pa + HIGHER_HALF_OFFSET;

        // Size: one bit per frame, rounded up to a page.
        const uint64_t top_phys_approx = PMM::get_total_memory();
        const usize    bitmap_sz =
            page_align_up((top_phys_approx / FRAME_SIZE / 8) + 1);

        PagingError err = g_vmm->m_kernel_as.register_vma(
            bitmap_virt, bitmap_sz,
            PageFlags::KernelRW | PageFlags::Global,
            VMAType::Kernel);
            
        if (!is_ok(err)) {
            kprintf("VMM: BITMAP REGISTRATION FAILED\n");
            kprintf("Error: ");
            kprintf(paging_error_to_string(err));
            kernel_panic("\nVMM: bitmap register_vma failed");
        }
    }

    p3_table[0] = PageTableEntry::make_empty();
    g_vmm->m_backend.flush_tlb_full();
    
    kprintf("VMM: initialised and kernel address space activated.\n");
}