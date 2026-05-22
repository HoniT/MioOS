// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef PAGING_HPP
#define PAGING_HPP

#include <mm/mm_defs.hpp>

namespace mem
{
    // Represents a single page
    union PageTableEntry {
        // Raw value
        uint64_t raw;

        // Bitmap
        struct {
            uint64_t present       : 1;
            uint64_t writable      : 1;
            uint64_t user          : 1;
            uint64_t write_through : 1;
            uint64_t cache_disable : 1;
            uint64_t accessed      : 1;
            uint64_t dirty         : 1;
            uint64_t huge_or_pat   : 1;
            uint64_t global        : 1;
            uint64_t available0    : 3;
            uint64_t pfn           : 40;
            uint64_t available1    : 11;
            uint64_t no_execute    : 1;
        } bits __attribute__((packed));

        constexpr PageTableEntry()                         noexcept : raw(0) {}
        explicit constexpr PageTableEntry(uint64_t r)      noexcept : raw(r) {}

        // Physical address accessors

        static constexpr uint64_t PFN_MASK = 0x000F'FFFF'FFFF'F000ULL;

        /// Extract the physical address embedded in this entry.
        [[nodiscard]] constexpr PhysAddr physical_address() const noexcept {
            return static_cast<PhysAddr>(raw & PFN_MASK);
        }

        /// Overwrite the PFN field, preserving all flag bits.
        constexpr void set_physical_address(PhysAddr pa) noexcept {
            raw = (raw & ~PFN_MASK) | (static_cast<uint64_t>(pa) & PFN_MASK);
        }


        [[nodiscard]] constexpr bool is_present()    const noexcept { return bits.present      != 0; }
        [[nodiscard]] constexpr bool is_writable()   const noexcept { return bits.writable     != 0; }
        [[nodiscard]] constexpr bool is_user()       const noexcept { return bits.user         != 0; }
        [[nodiscard]] constexpr bool is_huge()       const noexcept { return bits.huge_or_pat  != 0; }
        [[nodiscard]] constexpr bool is_global()     const noexcept { return bits.global       != 0; }
        [[nodiscard]] constexpr bool is_no_execute() const noexcept { return bits.no_execute   != 0; }
        [[nodiscard]] constexpr bool is_accessed()   const noexcept { return bits.accessed     != 0; }
        [[nodiscard]] constexpr bool is_dirty()      const noexcept { return bits.dirty        != 0; }
    
        /// Create an empty (not-present) entry.
        [[nodiscard]] static constexpr PageTableEntry make_empty() noexcept {
            return PageTableEntry{0};
        }

        /// Create an intermediate entry pointing to the next-level page table
        [[nodiscard]] static constexpr PageTableEntry make_table(
                PhysAddr next_phys,
                bool     user = false) noexcept {
            PageTableEntry e{};
            e.bits.present  = 1;
            e.bits.writable = 1;              // Allow A/D bit updates by hardware
            e.bits.user     = user ? 1 : 0;
            e.set_physical_address(next_phys);
            return e;
        }

        /// Create a 4 KiB leaf PTE (used in level-1 Page Tables).
        /// PageFlags are converted to hardware bits by apply_flags().
        [[nodiscard]] static constexpr PageTableEntry make_page(
                PhysAddr  phys,
                PageFlags flags) noexcept;

        /// Create a 2 MiB huge-page PDE (used in level-2 Page Directories).
        /// @pre  `phys` must be 2 MiB aligned.
        [[nodiscard]] static constexpr PageTableEntry make_huge_2m(
                PhysAddr  phys,
                PageFlags flags) noexcept;

        /// Create a 1 GiB huge-page PDPTE (used in level-3 PDPTs).
        /// @pre  `phys` must be 1 GiB aligned.
        [[nodiscard]] static constexpr PageTableEntry make_huge_1g(
                PhysAddr  phys,
                PageFlags flags) noexcept;
    };

    static_assert(sizeof(PageTableEntry)  == 8, "PageTableEntry must be 8 bytes");
    static_assert(alignof(PageTableEntry) == 8, "PageTableEntry must be 8-byte aligned");


    // Page tables
    struct alignas(PAGE_SIZE) PageTable {
        static constexpr usize ENTRY_COUNT = 512;

        PageTableEntry entries[ENTRY_COUNT];

        // Subscript operators for convenience.
        [[nodiscard]] constexpr PageTableEntry& operator[](usize i) noexcept {
            return entries[i];
        }
        [[nodiscard]] constexpr const PageTableEntry& operator[](usize i) const noexcept {
            return entries[i];
        }

        /// Zero all entries (marks every slot as not-present).
        void clear() noexcept {
            for (usize i = 0; i < ENTRY_COUNT; ++i)
                entries[i] = PageTableEntry::make_empty();
        }

        /// Return a pointer to the entry for index `i` without bounds checking.
        [[nodiscard]] constexpr PageTableEntry* entry_ptr(usize i) noexcept {
            return &entries[i];
        }
    };

    static_assert(sizeof(PageTable)  == PAGE_SIZE, "PageTable must be exactly 4096 bytes");
    static_assert(alignof(PageTable) == PAGE_SIZE, "PageTable must be page-aligned");

    using PML4Table = PageTable;   // Level-4: Page Map Level-4
    using PDPTable  = PageTable;   // Level-3: Page Directory Pointer Table
    using PDTable   = PageTable;   // Level-2: Page Directory
    using PTTable   = PageTable;   // Level-1: Page Table

    using PML4Entry = PageTableEntry;
    using PDPEntry  = PageTableEntry;
    using PDEntry   = PageTableEntry;
    using PTEntry   = PageTableEntry;

    /// Virtual address fields (indexes...)
    struct VirtualAddressFields {
        usize pml4_index;   // Bits [47:39] -> index into PML4Table
        usize pdpt_index;   // Bits [38:30] -> index into PDPTable
        usize pd_index;     // Bits [29:21] -> index into PDTable
        usize pt_index;     // Bits [20:12] -> index into PTTable
        usize page_offset;  // Bits [ 11:0] -> byte offset within page

        static constexpr usize INDEX_MASK  = 0x1FFu;   // 9-bit mask
        static constexpr usize OFFSET_MASK = 0xFFFu;   // 12-bit mask

        /// Decompose a virtual address into its constituent index fields.
        [[nodiscard]] static constexpr VirtualAddressFields decompose(VirtAddr va) noexcept {
            return VirtualAddressFields{
                .pml4_index  = (va >> 39) & INDEX_MASK,
                .pdpt_index  = (va >> 30) & INDEX_MASK,
                .pd_index    = (va >> 21) & INDEX_MASK,
                .pt_index    = (va >> 12) & INDEX_MASK,
                .page_offset = (va >>  0) & OFFSET_MASK,
            };
        }

        /// Recompose a canonical virtual address from index fields.
        [[nodiscard]] static constexpr VirtAddr compose(
                usize pml4, usize pdpt, usize pd, usize pt,
                usize offset = 0) noexcept {
            VirtAddr va = (static_cast<VirtAddr>(pml4)   << 39)
                        | (static_cast<VirtAddr>(pdpt)   << 30)
                        | (static_cast<VirtAddr>(pd)     << 21)
                        | (static_cast<VirtAddr>(pt)     << 12)
                        | static_cast<VirtAddr>(offset);
            // Sign-extend bit 47 into the upper 16 bits (canonical address form).
            if (va & (VirtAddr{1} << 47))
                va |= 0xFFFF'0000'0000'0000ULL;
            return va;
        }
    };

    namespace detail
    {
        [[nodiscard]] constexpr PageTableEntry apply_flags(
                PageTableEntry e, PageFlags flags) noexcept {
            //  R/W - writable iff PageFlags::Write is set.
            e.bits.writable      = has_flag(flags, PageFlags::Write)       ? 1u : 0u;
            //  U/S - user-accessible iff PageFlags::User is set.
            e.bits.user          = has_flag(flags, PageFlags::User)        ? 1u : 0u;
            //  PWT - write-through cache policy.
            e.bits.write_through = has_flag(flags, PageFlags::WriteThrough)? 1u : 0u;
            //  PCD - disable caching (MMIO).
            e.bits.cache_disable = has_flag(flags, PageFlags::NoCache)     ? 1u : 0u;
            //  G   - global (not flushed on CR3 reload).
            e.bits.global        = has_flag(flags, PageFlags::Global)      ? 1u : 0u;
            //  XD  - no-execute: SET when Execute flag is ABSENT.
            //        (NXE must be enabled in IA32_EFER for this to take effect.)
            e.bits.no_execute    = has_flag(flags, PageFlags::Execute)     ? 0u : 1u;
            return e;
        }
    } // namespace detail

    [[nodiscard]] constexpr PageTableEntry PageTableEntry::make_page(
            PhysAddr phys, PageFlags flags) noexcept {
        PageTableEntry e{};
        e.bits.present = 1;
        e.set_physical_address(phys);
        return detail::apply_flags(e, flags);
    }

    [[nodiscard]] constexpr PageTableEntry PageTableEntry::make_huge_2m(
            PhysAddr phys, PageFlags flags) noexcept {
        PageTableEntry e{};
        e.bits.present     = 1;
        e.bits.huge_or_pat = 1;   // PS = 1 -> level-2 entry is a leaf (2 MiB page)
        e.set_physical_address(phys);
        return detail::apply_flags(e, flags);
    }

    [[nodiscard]] constexpr PageTableEntry PageTableEntry::make_huge_1g(
            PhysAddr phys, PageFlags flags) noexcept {
        PageTableEntry e{};
        e.bits.present     = 1;
        e.bits.huge_or_pat = 1;   // PS = 1 -> level-3 entry is a leaf (1 GiB page)
        e.set_physical_address(phys);
        return detail::apply_flags(e, flags);
    }

    struct PagingContext {
        PhysAddr value = 0;

        [[nodiscard]] static constexpr PagingContext invalid() noexcept {
            return PagingContext{0};
        }
        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return value != 0;
        }

        // Equality is based purely on the physical address value so that
        // AddressSpace can compare contexts without knowing their internals.
        [[nodiscard]] constexpr bool operator==(PagingContext o) const noexcept { return value == o.value; }
        [[nodiscard]] constexpr bool operator!=(PagingContext o) const noexcept { return value != o.value; }
    };

    /// Paging errors
    enum class PagingError : uint8_t {
        Success         = 0,
        OutOfMemory,        // Physical page allocator returned null
        AlreadyMapped,      // A mapping already exists at the target address
        NotMapped,          // No mapping exists at the given address
        InvalidAlignment,   // Address or size is not page-aligned
        InvalidContext,     // PagingContext::is_valid() == false
        InvalidAddress,     // Address outside the valid canonical range
        HardwareFault,      // Architecture-specific hardware error
    };

    [[nodiscard]] constexpr bool is_ok(PagingError e) noexcept {
        return e == PagingError::Success;
    }


    class PagingBackend final {
    public:

        /// Construct the backend.
        /// @param hhdm_offset  Virtual base of the higher-half direct map.
        ///                     Physical address P is accessible at hhdm_offset + P.
        ///                     Defaults to HHDM_BASE from mm_types.hpp.
        explicit PagingBackend(VirtAddr hhdm_offset = HHDM_BASE) noexcept;
        PagingBackend(PhysAddr boot_pml4_phys,
                        VirtAddr hhdm_offset) noexcept;

        ~PagingBackend() noexcept = default;

        PagingBackend(const PagingBackend&)            = delete;
        PagingBackend& operator=(const PagingBackend&) = delete;

        // IPagingBackend Interface

        [[nodiscard]] PagingContext create_context()                          noexcept;
        void                        destroy_context(PagingContext ctx)        noexcept;

        [[nodiscard]] PagingError   map_page(PagingContext ctx, VirtAddr virt,
                                            PhysAddr phys, PageFlags flags)  noexcept;
        [[nodiscard]] PagingError   unmap_page(PagingContext ctx, VirtAddr v)  noexcept;
        [[nodiscard]] PagingError   protect_page(PagingContext ctx, VirtAddr v,
                                                PageFlags flags)              noexcept;

        [[nodiscard]] PhysAddr      translate(PagingContext ctx,
                                            VirtAddr virt) const             noexcept;

        void                        activate(PagingContext ctx)                noexcept;
        void                        flush_tlb_page(VirtAddr virt)             noexcept;
        void                        flush_tlb_full()                          noexcept;

        void                        share_kernel_mappings(PagingContext ctx)  noexcept;
        [[nodiscard]] PagingContext kernel_context() const                    noexcept;

        // x86_64-Specific Extras

        /// Return a pointer to the kernel's PML4 table via the HHDM.
        [[nodiscard]] PML4Table* kernel_pml4() const noexcept;

        /// Read the currently active context from CR3 (strips PCID bits).
        [[nodiscard]] static PagingContext current_context() noexcept;

        /// Query CPUID to check whether the NX/XD bit is supported.
        /// If false, no_execute bits in PTEs are silently ignored by hardware.
        [[nodiscard]] static bool nx_supported() noexcept;

        /// Query CPUID.80000001H:EDX[26] for 1 GiB huge page support.
        [[nodiscard]] static bool huge_1g_supported() noexcept;

        // ── HHDM Helpers (public for use by early boot code) ─────────────────────

        /// Convert a physical address to a kernel virtual address via the HHDM.
        [[nodiscard]] PageTable* phys_to_virt(PhysAddr phys) const noexcept {
            return reinterpret_cast<PageTable*>(m_hhdm_offset + phys);
        }

        /// Convert a kernel virtual address (in HHDM range) back to physical.
        [[nodiscard]] PhysAddr virt_to_phys(const PageTable* virt) const noexcept {
            return reinterpret_cast<VirtAddr>(virt) - m_hhdm_offset;
        }

    private:
        // Members

        VirtAddr        m_hhdm_offset;       // HHDM virtual base address
        PhysAddr        m_kernel_pml4_phys;  // Physical address of kernel PML4

        // Private Helpers

        /// Allocate and zero a single physical page for use as a page table.
        /// Returns 0 on allocation failure.
        [[nodiscard]] PhysAddr alloc_page_table() noexcept;

        /// Ensure the given `entry` points to a valid next-level page table.
        /// If the entry is not present, allocates a new page table and installs it.
        ///
        /// @param entry  Reference to the parent-level page table entry.
        /// @param user   Whether the path must be user-accessible.
        /// @returns      Pointer to the child PageTable (via HHDM), or nullptr.
        [[nodiscard]] PageTable* get_or_create_subtable(
            PageTableEntry& entry, bool user) noexcept;

        /// Recursively free all user-space page tables rooted at `pml4`.
        void free_user_tables(PML4Table* pml4) noexcept;

        // Inline Assembly Wrappers

        static void     write_cr3(uint64_t val) noexcept;
        static uint64_t read_cr3()              noexcept;

        /// Issue `invlpg [vaddr]` to invalidate a single TLB entry.
        static void invlpg(VirtAddr vaddr) noexcept;

        /// Read a MSR (Model-Specific Register).
        static uint64_t rdmsr(uint32_t msr) noexcept;

        /// Write a MSR.
        static void wrmsr(uint32_t msr, uint64_t val) noexcept;

        /// Execute CPUID and return the value of the requested register.
        enum class CpuidReg : uint8_t { EAX, EBX, ECX, EDX };
        static uint32_t cpuid(uint32_t leaf, uint32_t subleaf,
                            CpuidReg reg) noexcept;
    };
} // namespace mem


#endif // PAGING_HPP
