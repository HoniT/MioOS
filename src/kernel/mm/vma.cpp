// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <mm/vma.hpp>

namespace mem
{
    VMATree::~VMATree() {
        
    }
    
    VMArea* VMATree::insert(uintptr_t start, size_t size, PageFlags flags, VMAType type) {
        
    }
    
    void VMATree::remove(uintptr_t start, size_t size) {
        
    }

    VMArea* VMATree::find(uintptr_t address) {
        
    }

    bool VMATree::is_range_free(uintptr_t start, size_t size) {
        
    }
} // namespace mem