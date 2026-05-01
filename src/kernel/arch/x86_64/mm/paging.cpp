// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <arch/x86_64/mm/paging.hpp>

namespace x86_64::mem {

    // Assembly wrappers
    void X86_64PagingBackend::write_cr3(uint64_t val) noexcept {
        asm volatile("mov %0, %%cr3" : : "r"(val) : "memory");
    }

    uint64_t X86_64PagingBackend::read_cr3() noexcept {
        uint64_t val;
        asm volatile("mov %%cr3, %0" : "=r"(val));
        return val;
    }

    void X86_64PagingBackend::invlpg(VirtAddr vaddr) noexcept {
        asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
    }

    uint64_t X86_64PagingBackend::rdmsr(uint32_t msr) noexcept {
        uint32_t lo, hi;
        asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
        return (static_cast<uint64_t>(hi) << 32) | lo;
    }

    void X86_64PagingBackend::wrmsr(uint32_t msr, uint64_t val) noexcept {
        asm volatile("wrmsr"
            : : "c"(msr),
                "a"(static_cast<uint32_t>(val)),
                "d"(static_cast<uint32_t>(val >> 32)));
    }

    uint32_t X86_64PagingBackend::cpuid(
            uint32_t leaf, uint32_t subleaf, CpuidReg reg) noexcept {
        uint32_t eax, ebx, ecx, edx;
        asm volatile("cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(leaf), "c"(subleaf));
        switch (reg) {
            case CpuidReg::EAX: return eax;
            case CpuidReg::EBX: return ebx;
            case CpuidReg::ECX: return ecx;
            case CpuidReg::EDX: return edx;
        }
        return 0;
    }

    X86_64PagingBackend::X86_64PagingBackend(VirtAddr hhdm_offset) noexcept
        : m_hhdm_offset(hhdm_offset)
        , m_kernel_pml4_phys(0)
    {
        m_kernel_pml4_phys = alloc_page_table();
    }

    X86_64PagingBackend::X86_64PagingBackend(PhysAddr boot_pml4_phys,
                                            VirtAddr hhdm_offset) noexcept
        : m_hhdm_offset     (hhdm_offset)
        , m_kernel_pml4_phys(boot_pml4_phys) { }

    /// @brief Allocates a page table
    /// @return 
    PhysAddr X86_64PagingBackend::alloc_page_table() noexcept {
        PhysAddr pa = reinterpret_cast<PhysAddr>(::mem::PMM::alloc_frame());
        if (!pa) return 0;

        // Zero the page so every entry starts as not-present.
        phys_to_virt(pa)->clear();
        return pa;
    }

    PagingContext X86_64PagingBackend::create_context() noexcept {
        PhysAddr pml4_phys = alloc_page_table();
        if (!pml4_phys) return PagingContext::invalid();

        PagingContext ctx{pml4_phys};
        share_kernel_mappings(ctx);
        return ctx;
    }

    void X86_64PagingBackend::destroy_context(PagingContext ctx) noexcept {
        if (!ctx.is_valid()) return;
        auto* pml4 = phys_to_virt(ctx.value);
        free_user_tables(pml4);
        ::mem::PMM::free_frame(reinterpret_cast<void*>(ctx.value));
    }

    void X86_64PagingBackend::share_kernel_mappings(PagingContext ctx) noexcept {
        if (!ctx.is_valid() || !m_kernel_pml4_phys) return;

        auto* dst = phys_to_virt(ctx.value);
        auto* src = phys_to_virt(m_kernel_pml4_phys);

        // The kernel occupies the upper half: PML4 entries [256, 511].
        // Copy the raw 64-bit values — this propagates any kernel mappings
        // established before this context was created.
        for (usize i = 256; i < PageTable::ENTRY_COUNT; ++i)
            dst->entries[i] = src->entries[i];
    }

    PagingContext X86_64PagingBackend::kernel_context() const noexcept {
        return PagingContext{m_kernel_pml4_phys};
    }

    PageTable* X86_64PagingBackend::get_or_create_subtable(
        PageTableEntry& entry, bool user) noexcept {
        if (entry.is_present()) {
            // Entry exists. If it's a huge-page leaf, the caller made a mistake.
            // Huge pages are detected by the caller before descending.
            return phys_to_virt(entry.physical_address());
        }

        // Allocate a new page table for the next level.
        PhysAddr child_phys = alloc_page_table();
        if (!child_phys) return nullptr;

        entry = PageTableEntry::make_table(child_phys, user);
        return phys_to_virt(child_phys);
    }


    PagingError X86_64PagingBackend::map_page(
            PagingContext ctx,
            VirtAddr      virt,
            PhysAddr      phys,
            PageFlags     flags) noexcept {

        if (!ctx.is_valid())           return PagingError::InvalidContext;
        if (!is_page_aligned(virt))    return PagingError::InvalidAlignment;
        if (!is_page_aligned(phys))    return PagingError::InvalidAlignment;

        const bool user = has_flag(flags, PageFlags::User);
        const auto va   = VirtualAddressFields::decompose(virt);

        // Level 4: PML4
        auto* pml4 = phys_to_virt(ctx.value);
        auto* pdpt = get_or_create_subtable((*pml4)[va.pml4_index], user);
        if (!pdpt) return PagingError::OutOfMemory;

        // Level 3: PDPT
        auto& pdpt_entry = (*pdpt)[va.pdpt_index];
        if (pdpt_entry.is_present() && pdpt_entry.is_huge())
            return PagingError::AlreadyMapped;   // Covered by a 1 GiB huge page

        auto* pd = get_or_create_subtable(pdpt_entry, user);
        if (!pd) return PagingError::OutOfMemory;

        // Level 2: PD
        auto& pd_entry = (*pd)[va.pd_index];
        if (pd_entry.is_present() && pd_entry.is_huge())
            return PagingError::AlreadyMapped;   // Covered by a 2 MiB huge page

        auto* pt = get_or_create_subtable(pd_entry, user);
        if (!pt) return PagingError::OutOfMemory;

        // Level 1: PT (leaf)
        auto& pte = (*pt)[va.pt_index];
        if (pte.is_present()) return PagingError::AlreadyMapped;

        pte = PageTableEntry::make_page(phys, flags);
        return PagingError::Success;
    }

    PagingError X86_64PagingBackend::unmap_page(
            PagingContext ctx, VirtAddr virt) noexcept {

        if (!ctx.is_valid())        return PagingError::InvalidContext;
        if (!is_page_aligned(virt)) return PagingError::InvalidAlignment;

        const auto va = VirtualAddressFields::decompose(virt);

        auto* pml4 = phys_to_virt(ctx.value);
        if (!(*pml4)[va.pml4_index].is_present()) return PagingError::NotMapped;

        auto* pdpt = phys_to_virt((*pml4)[va.pml4_index].physical_address());
        if (!(*pdpt)[va.pdpt_index].is_present()) return PagingError::NotMapped;

        auto* pd = phys_to_virt((*pdpt)[va.pdpt_index].physical_address());
        if (!(*pd)[va.pd_index].is_present())     return PagingError::NotMapped;

        auto* pt = phys_to_virt((*pd)[va.pd_index].physical_address());
        auto& pte = (*pt)[va.pt_index];
        if (!pte.is_present()) return PagingError::NotMapped;

        pte = PageTableEntry::make_empty();
        invlpg(virt);
        return PagingError::Success;
    }

    PagingError X86_64PagingBackend::protect_page(
            PagingContext ctx, VirtAddr virt, PageFlags new_flags) noexcept {

        if (!ctx.is_valid())        return PagingError::InvalidContext;
        if (!is_page_aligned(virt)) return PagingError::InvalidAlignment;

        const auto va = VirtualAddressFields::decompose(virt);

        auto* pml4 = phys_to_virt(ctx.value);
        if (!(*pml4)[va.pml4_index].is_present()) return PagingError::NotMapped;
        auto* pdpt = phys_to_virt((*pml4)[va.pml4_index].physical_address());
        if (!(*pdpt)[va.pdpt_index].is_present()) return PagingError::NotMapped;
        auto* pd = phys_to_virt((*pdpt)[va.pdpt_index].physical_address());
        if (!(*pd)[va.pd_index].is_present())     return PagingError::NotMapped;
        auto* pt = phys_to_virt((*pd)[va.pd_index].physical_address());
        auto& pte = (*pt)[va.pt_index];
        if (!pte.is_present()) return PagingError::NotMapped;

        // Preserve the physical address; replace the permission bits.
        PhysAddr phys = pte.physical_address();
        pte = PageTableEntry::make_page(phys, new_flags);
        invlpg(virt);
        return PagingError::Success;
    }

    PhysAddr X86_64PagingBackend::translate(
            PagingContext ctx, VirtAddr virt) const noexcept {

        if (!ctx.is_valid()) return 0;

        const auto va = VirtualAddressFields::decompose(virt);

        auto* pml4 = phys_to_virt(ctx.value);
        const auto& e4 = (*pml4)[va.pml4_index];
        if (!e4.is_present()) return 0;

        auto* pdpt = phys_to_virt(e4.physical_address());
        const auto& e3 = (*pdpt)[va.pdpt_index];
        if (!e3.is_present()) return 0;
        if (e3.is_huge()) {
            // 1 GiB page: physical base + offset within 1 GiB page.
            return e3.physical_address() | (virt & (PAGE_SIZE_1G - 1));
        }

        auto* pd = phys_to_virt(e3.physical_address());
        const auto& e2 = (*pd)[va.pd_index];
        if (!e2.is_present()) return 0;
        if (e2.is_huge()) {
            // 2 MiB page: physical base + offset within 2 MiB page.
            return e2.physical_address() | (virt & (PAGE_SIZE_2M - 1));
        }

        auto* pt = phys_to_virt(e2.physical_address());
        const auto& e1 = (*pt)[va.pt_index];
        if (!e1.is_present()) return 0;

        // 4 KiB page: physical base + 12-bit page offset.
        return e1.physical_address() | va.page_offset;
    }

    void X86_64PagingBackend::activate(PagingContext ctx) noexcept {
        if (ctx.is_valid())
            write_cr3(ctx.value);
    }

    PagingContext X86_64PagingBackend::current_context() noexcept {
        // CR3[11:0] contain flags (PCID etc.); the PML4 base is in [63:12].
        constexpr uint64_t CR3_ADDR_MASK = ~static_cast<uint64_t>(0xFFF);
        return PagingContext{read_cr3() & CR3_ADDR_MASK};
    }

    void X86_64PagingBackend::flush_tlb_page(VirtAddr virt) noexcept {
        invlpg(virt);
    }

    void X86_64PagingBackend::flush_tlb_full() noexcept {
        // Reload CR3 with itself; flushes all non-global TLB entries.
        write_cr3(read_cr3());
    }


    bool X86_64PagingBackend::nx_supported() noexcept {
        constexpr uint32_t NX_BIT = 1u << 20;
        return (cpuid(0x8000'0001, 0, CpuidReg::EDX) & NX_BIT) != 0;
    }

    bool X86_64PagingBackend::huge_1g_supported() noexcept {
        constexpr uint32_t PDPE1GB = 1u << 26;
        return (cpuid(0x8000'0001, 0, CpuidReg::EDX) & PDPE1GB) != 0;
    }

    PML4Table* X86_64PagingBackend::kernel_pml4() const noexcept {
        return phys_to_virt(m_kernel_pml4_phys);
    }


    void X86_64PagingBackend::free_user_tables(PML4Table* pml4) noexcept {
        // Only iterate user-space half
        for (usize i4 = 0; i4 < 256; ++i4) {
            auto& e4 = (*pml4)[i4];
            if (!e4.is_present()) continue;

            auto* pdpt = phys_to_virt(e4.physical_address());
            for (usize i3 = 0; i3 < PageTable::ENTRY_COUNT; ++i3) {
                auto& e3 = (*pdpt)[i3];
                if (!e3.is_present()) continue;
                if (e3.is_huge()) { e3 = PageTableEntry::make_empty(); continue; }

                auto* pd = phys_to_virt(e3.physical_address());
                for (usize i2 = 0; i2 < PageTable::ENTRY_COUNT; ++i2) {
                    auto& e2 = (*pd)[i2];
                    if (!e2.is_present()) continue;
                    if (e2.is_huge()) { e2 = PageTableEntry::make_empty(); continue; }

                    auto* pt = phys_to_virt(e2.physical_address());
                    pt->clear();
                    ::mem::PMM::free_frame(reinterpret_cast<void*>(e2.physical_address()));
                    e2 = PageTableEntry::make_empty();
                }
                ::mem::PMM::free_frame(reinterpret_cast<void*>(e3.physical_address()));
                e3 = PageTableEntry::make_empty();
            }
            ::mem::PMM::free_frame(reinterpret_cast<void*>(e4.physical_address()));
            e4 = PageTableEntry::make_empty();
        }
    }

} // namespace x86_64::mem
