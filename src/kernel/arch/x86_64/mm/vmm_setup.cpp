// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <arch/x86_64/mm/vmm_setup.hpp>
#include <mm/vma.hpp>
#include <arch/x86_64/mm/paging.hpp>
#include <mm/mm_types.hpp>
#include <mm/address_space.hpp>
#include <arch/x86_64/entry.hpp>

extern "C" arch::mem::PageTable p4_table;
extern "C" arch::mem::PageTable p3_table;

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

alignas(mem::VMMState) static uint8_t g_vmm_storage[sizeof(mem::VMMState)];
mem::VMMState* g_vmm = nullptr;

void mem::vmm_init() noexcept {
    const PhysAddr boot_pml4_phys =
        reinterpret_cast<VirtAddr>(&p4_table) - KERNEL_VIRT_BASE;

    g_vmm = new (g_vmm_storage) mem::VMMState(boot_pml4_phys);

    if (!g_vmm->m_backend.kernel_context().is_valid())
    kpanic("VMM: kernel PML4 allocation failed — PMM has no free frames");
    
    if (!g_vmm->m_kernel_as.context().is_valid())
    kpanic("VMM: kernel AddressSpace context invalid after construction");
    
    if (!arch::mem::X86_64PagingBackend::nx_supported())
    if (klog) klog("VMM: WARNING — NX/XD bit not supported by CPU.");

    auto reg = [&](VirtAddr vstart, VirtAddr vend,
                    PageFlags flags, const char* name) noexcept {
        const VirtAddr base = page_align_down(vstart);
        const usize    sz   = page_align_up(vend) - base;

        PagingError err = g_vmm->m_kernel_as.register_vma(
            base, sz, flags | PageFlags::Global, VMAType::Kernel);

        if (!is_ok(err)) { 
            klog("--- VMA REGISTRATION FAILED ---");
            klog("Region:");
            klog(name);
            klog("Error:");
            klog(paging_error_to_string(err));
            kpanic("VMM: register_vma failed"); 
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
            (kernel_end_pa + mem::FRAME_SIZE - 1) & ~(mem::FRAME_SIZE - 1);
        const VirtAddr bitmap_virt    = bitmap_pa + mem::HIGHER_HALF_OFFSET;

        // Size: one bit per frame, rounded up to a page.
        const uint64_t top_phys_approx = mem::PMM::get_total_memory();
        const usize    bitmap_sz =
            page_align_up((top_phys_approx / mem::FRAME_SIZE / 8) + 1);

        PagingError err = g_vmm->m_kernel_as.register_vma(
            bitmap_virt, bitmap_sz,
            PageFlags::KernelRW | PageFlags::Global,
            VMAType::Kernel);
            
        if (!is_ok(err)) {
            klog("--- BITMAP REGISTRATION FAILED ---");
            klog("Error:");
            klog(paging_error_to_string(err));
            kpanic("VMM: bitmap register_vma failed");
        }
    }

    p3_table[0] = arch::mem::PageTableEntry::make_empty();
    g_vmm->m_backend.flush_tlb_full();
    
    if (klog) klog("VMM: initialised and kernel address space activated.");
}