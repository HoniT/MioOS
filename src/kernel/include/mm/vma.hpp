// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once

#ifndef VMA_HPP
#define VMA_HPP

#include <mm/mm_defs.hpp>

namespace mem {
    /// @brief Virtual Memory Area
    class VMArea {
    public:
        VirtAddr start = 0;
        usize    size  = 0;

        // Region Attributes

        PageFlags flags = PageFlags::None;
        VMAType   type  = VMAType::Anonymous;

        //  Managed exclusively by VMATree

        VMArea*  avl_left   = nullptr;
        VMArea*  avl_right  = nullptr;
        VMArea*  avl_parent = nullptr;
        int32_t  avl_height = 1;      


        constexpr VMArea() noexcept = default;

        constexpr VMArea(VirtAddr start_, usize size_,
                        PageFlags flags_, VMAType type_) noexcept
            : start(start_), size(size_), flags(flags_), type(type_) {}

        // Geometry

        /// One-past-the-end virtual address of this region.
        [[nodiscard]] constexpr VirtAddr end() const noexcept {
            return start + size;
        }

        /// True iff `addr` falls within [start, end).
        [[nodiscard]] constexpr bool contains(VirtAddr addr) const noexcept {
            return addr >= start && addr < end();
        }

        /// True iff the range [s, s+sz) overlaps [start, end) at all.
        [[nodiscard]] constexpr bool overlaps(VirtAddr s, usize sz) const noexcept {
            return s < end() && (s + sz) > start;
        }

        /// True iff geometry is valid: non-zero size, both addresses page-aligned.
        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return size > 0
                && is_page_aligned(start)
                && is_page_aligned(size);
        }

        [[nodiscard]] constexpr bool is_readable()   const noexcept { return has_flag(flags, PageFlags::Read);    }
        [[nodiscard]] constexpr bool is_writable()   const noexcept { return has_flag(flags, PageFlags::Write);   }
        [[nodiscard]] constexpr bool is_executable() const noexcept { return has_flag(flags, PageFlags::Execute); }
        [[nodiscard]] constexpr bool is_user()       const noexcept { return has_flag(flags, PageFlags::User);    }
        [[nodiscard]] constexpr bool is_global()     const noexcept { return has_flag(flags, PageFlags::Global);  }
        [[nodiscard]] constexpr bool is_mmio()       const noexcept { return type == VMAType::MMIO;               }
        [[nodiscard]] constexpr bool is_stack()      const noexcept { return type == VMAType::Stack;              }
    };

    /// AVL tree for VMAs
    class VMATree {
    public:
        VMATree() noexcept = default;

        VMATree(const VMATree&)            = delete;
        VMATree& operator=(const VMATree&) = delete;
        VMATree(VMATree&&)                 = delete;
        VMATree& operator=(VMATree&&)      = delete;

        // Mutation

        /// Insert a vma into the tree.
        bool insert(VMArea* vma) noexcept;

        /// Remove a vma from the tree (caller retains ownership of the node).
        bool remove(VMArea* vma) noexcept;

        // Lookup

        [[nodiscard]] VMArea* find(VirtAddr addr) const noexcept;

        [[nodiscard]] VMArea* find_predecessor(VirtAddr addr) const noexcept;

        /// Find any VMA that overlaps the range [addr, addr + size)
        [[nodiscard]] VMArea* find_overlap(VirtAddr addr, usize size) const noexcept;

        /// Scan the address space for an unmapped gap of at least `size` bytes,
        /// starting the search from `hint` and stopping before `limit`.
        [[nodiscard]] VirtAddr find_free_region(VirtAddr hint,
                                                usize    size,
                                                VirtAddr limit) const noexcept;

        // Iteration

        template<typename Fn>
        void for_each(Fn&& fn) const noexcept {
            in_order(m_root, fn);
        }

        // Tree State

        [[nodiscard]] bool    empty() const noexcept { return m_root == nullptr; }
        [[nodiscard]] usize   count() const noexcept { return m_count; }
        [[nodiscard]] VMArea* root()  const noexcept { return m_root; }

        // Debugging

        /// Validate AVL invariants (height, BST order, no overlaps)
        [[nodiscard]] bool validate() const noexcept;

    private:
        VMArea* m_root  = nullptr;
        usize   m_count = 0;

        // Internal AVL Helpers

        [[nodiscard]] static int32_t  node_height(const VMArea* n)     noexcept;
        [[nodiscard]] static int32_t  balance_factor(const VMArea* n)  noexcept;
        static void                   update_height(VMArea* n)         noexcept;
        void                          set_child(VMArea* parent,
                                                VMArea* old_child,
                                                VMArea* new_child)     noexcept;

        [[nodiscard]] VMArea* rotate_right(VMArea* y)                  noexcept;
        [[nodiscard]] VMArea* rotate_left(VMArea* x)                   noexcept;
        [[nodiscard]] VMArea* rebalance(VMArea* n)                     noexcept;

        // Returns the new root of the subtree after insert.
        [[nodiscard]] VMArea* insert_at(VMArea* subtree, VMArea* vma)  noexcept;
        [[nodiscard]] static VMArea* successor(VMArea* n)              noexcept;

        // Iterative in-order traversal
        template<typename Fn>
        void in_order(VMArea* node, Fn& fn) const noexcept {
            VMArea* curr = node;
            VMArea* stack[64];
            int     top = 0;

            while (curr || top > 0) {
                while (curr) {
                    stack[top++] = curr;
                    curr = curr->avl_left;
                }
                curr = stack[--top];
                fn(*curr);
                curr = curr->avl_right;
            }
        }
    };

    class VMASlabAllocator {
    public:
        static constexpr usize SLOTS_PER_PAGE = PAGE_SIZE / sizeof(VMArea); // 73
        static constexpr usize MAX_SLAB_PAGES = 14; // 14 × 73 = 1022 max VMArea nodes

        /// @param hhdm_base  Same HHDM offset used by the paging backend.
        ///                   Required to dereference physical slab pages as virtual
        ///                   addresses.  Pass 0 for an identity-mapped setup.
        explicit VMASlabAllocator(VirtAddr hhdm_base) noexcept;
        ~VMASlabAllocator() noexcept;

        VMASlabAllocator(const VMASlabAllocator&)            = delete;
        VMASlabAllocator& operator=(const VMASlabAllocator&) = delete;

        /// Allocate one VMArea slot. Returns zero-constructed VMArea* or nullptr.
        [[nodiscard]] VMArea* allocate() noexcept;

        /// Return a VMArea slot to the freelist.
        void deallocate(VMArea* vma) noexcept;

        [[nodiscard]] usize total_slots()     const noexcept;
        [[nodiscard]] usize free_slots()      const noexcept;
        [[nodiscard]] usize allocated_slots() const noexcept;
        [[nodiscard]] usize slab_page_count() const noexcept { return m_page_count; }

    private:
        struct FreeNode { FreeNode* next; };
        static_assert(sizeof(FreeNode)  <= sizeof(VMArea));
        static_assert(alignof(FreeNode) <= alignof(VMArea));

        VirtAddr  m_hhdm_base;
        FreeNode* m_freelist    = nullptr;
        usize    m_page_count  = 0;
        usize    m_free_count  = 0;
        usize    m_total_slots = 0;
        PhysAddr  m_pages[MAX_SLAB_PAGES]{};

        [[nodiscard]] bool expand() noexcept;

        [[nodiscard]] void* phys_to_virt(PhysAddr pa) const noexcept {
            return reinterpret_cast<void*>(m_hhdm_base + pa);
        }
    };

    // Trampolines matching AddressSpace::VMAAllocFn / VMAFreeFn signatures.
    inline VMArea* vma_slab_alloc(void* ctx) noexcept {
        return static_cast<VMASlabAllocator*>(ctx)->allocate();
    }
    inline void vma_slab_free(void* ctx, VMArea* vma) noexcept {
        static_cast<VMASlabAllocator*>(ctx)->deallocate(vma);
    }
} // namespace mem

#endif // VMA_HPP
