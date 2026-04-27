// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#include <mm/vma.hpp>

using mem::VMATree;
using mem::VMArea;

int32_t VMATree::node_height(const VMArea* n) noexcept {
    return n ? n->avl_height : 0;
}

int32_t VMATree::balance_factor(const VMArea* n) noexcept {
    if (!n) return 0;
    return node_height(n->avl_left) - node_height(n->avl_right);
}

void VMATree::update_height(VMArea* n) noexcept {
    if (!n) return;
    int32_t lh = node_height(n->avl_left);
    int32_t rh = node_height(n->avl_right);
    n->avl_height = 1 + (lh > rh ? lh : rh);
}


void VMATree::set_child(VMArea* parent,
                         VMArea* old_child,
                         VMArea* new_child) noexcept {
    if (!parent) {
        // old_child was the root — update the tree root.
        m_root = new_child;
    } else if (parent->avl_left == old_child) {
        parent->avl_left = new_child;
    } else {
        parent->avl_right = new_child;
    }
    if (new_child)
        new_child->avl_parent = parent;
}


VMArea* VMATree::rotate_right(VMArea* y) noexcept {
    VMArea* x  = y->avl_left;
    VMArea* B  = x->avl_right;
    VMArea* gp = y->avl_parent;

    // Perform rotation.
    x->avl_right = y;
    y->avl_left  = B;

    // Fix parent pointers.
    y->avl_parent = x;
    if (B) B->avl_parent = y;
    x->avl_parent = gp;

    // Update the grandparent's child pointer.
    set_child(gp, y, x);

    // Recompute heights bottom-up (y is now deeper than x).
    update_height(y);
    update_height(x);

    return x;
}


VMArea* VMATree::rotate_left(VMArea* x) noexcept {
    VMArea* y  = x->avl_right;
    VMArea* B  = y->avl_left;
    VMArea* gp = x->avl_parent;

    // Perform rotation.
    y->avl_left  = x;
    x->avl_right = B;

    // Fix parent pointers.
    x->avl_parent = y;
    if (B) B->avl_parent = x;
    y->avl_parent = gp;

    // Update the grandparent's child pointer.
    set_child(gp, x, y);

    update_height(x);
    update_height(y);

    return y;
}

VMArea* VMATree::rebalance(VMArea* n) noexcept {
    update_height(n);

    int32_t bf = balance_factor(n);

    // Left-heavy
    if (bf > 1) {
        if (balance_factor(n->avl_left) < 0)
            (void)rotate_left(n->avl_left);
        return rotate_right(n);
    }

    // Right-heavy
    if (bf < -1) {
        if (balance_factor(n->avl_right) > 0)
            (void)rotate_right(n->avl_right);
        return rotate_left(n);
    }

    // Already balanced — return unchanged.
    return n;
}

VMArea* VMATree::insert_at(VMArea* subtree, VMArea* vma) noexcept {
    if (!subtree) {
        // Base case: empty slot — place the new node here.
        vma->avl_left   = nullptr;
        vma->avl_right  = nullptr;
        vma->avl_height = 1;
        // avl_parent is set by the caller's set_child / initial assignment.
        return vma;
    }

    if (vma->start < subtree->start) {
        VMArea* new_left = insert_at(subtree->avl_left, vma);
        subtree->avl_left = new_left;
        new_left->avl_parent = subtree;
    } else {
        // vma->start > subtree->start (caller guarantees no duplicates).
        VMArea* new_right = insert_at(subtree->avl_right, vma);
        subtree->avl_right = new_right;
        new_right->avl_parent = subtree;
    }

    return rebalance(subtree);
}

VMArea* VMATree::successor(VMArea* n) noexcept {
    VMArea* curr = n->avl_right;
    while (curr && curr->avl_left)
        curr = curr->avl_left;
    return curr;
}

bool VMATree::insert(VMArea* vma) noexcept {
    if (!vma || !vma->is_valid()) return false;

    vma->avl_left   = nullptr;
    vma->avl_right  = nullptr;
    vma->avl_parent = nullptr;
    vma->avl_height = 1;

    if (!m_root) {
        m_root = vma;
        ++m_count;
        return true;
    }

    m_root = insert_at(m_root, vma);
    m_root->avl_parent = nullptr;
    ++m_count;
    return true;
}

bool VMATree::remove(VMArea* vma) noexcept {
    if (!vma) return false;

    VMArea* rebalance_start = nullptr; // Where to begin the upward rebalance pass.

    if (vma->avl_left && vma->avl_right) {
        VMArea* S  = successor(vma);
        VMArea* Sp = S->avl_parent;   // S's current parent (may be vma itself)
        VMArea* Sr = S->avl_right;    // S's only possible child

        // Unlink S from its current parent
        if (Sp == vma) {
            rebalance_start = S;
        } else {
            Sp->avl_left = Sr;
            if (Sr) Sr->avl_parent = Sp;
            // S will inherit vma's right subtree.
            S->avl_right = vma->avl_right;
            vma->avl_right->avl_parent = S;
            rebalance_start = Sp;
        }

        // Splice S into vma's position
        S->avl_left          = vma->avl_left;
        vma->avl_left->avl_parent = S;
        S->avl_height        = vma->avl_height;

        // Update S's parent (vma's parent now points to S).
        set_child(vma->avl_parent, vma, S);
        S->avl_parent = vma->avl_parent;

    } else {
        // Zero- or one-child case
        VMArea* child = vma->avl_left ? vma->avl_left : vma->avl_right;

        set_child(vma->avl_parent, vma, child);
        if (child) child->avl_parent = vma->avl_parent;

        rebalance_start = vma->avl_parent;
    }

    // Walk up and rebalance
    VMArea* curr = rebalance_start;
    while (curr) {
        VMArea* parent = curr->avl_parent;
        (void)rebalance(curr);
        curr = parent;
    }

    // Clear the removed node's links
    vma->avl_left   = nullptr;
    vma->avl_right  = nullptr;
    vma->avl_parent = nullptr;
    vma->avl_height = 1;

    --m_count;
    return true;
}

VMArea* VMATree::find(VirtAddr addr) const noexcept {
    VMArea* curr = m_root;
    while (curr) {
        if (addr < curr->start) {
            curr = curr->avl_left;
        } else if (addr >= curr->end()) {
            curr = curr->avl_right;
        } else {
            return curr;
        }
    }
    return nullptr;
}

VMArea* VMATree::find_predecessor(VirtAddr addr) const noexcept {
    VMArea* curr = m_root;
    VMArea* best = nullptr;

    while (curr) {
        if (curr->start <= addr) {
            best = curr;
            curr = curr->avl_right;
        } else {
            curr = curr->avl_left;
        }
    }
    return best;
}

VMArea* VMATree::find_overlap(VirtAddr addr, usize size) const noexcept {
    if (size == 0) return nullptr;

    const VirtAddr query_end = addr + size;

    VMArea* stack[64];
    int top = 0;

    if (m_root) stack[top++] = m_root;

    while (top > 0) {
        VMArea* n = stack[--top];

        // Does this node overlap [addr, query_end)?
        if (n->start < query_end && n->end() > addr)
            return n;

        // Prune: if query_end <= n->start, nothing in the right subtree
        // can overlap (all starts are even larger).
        if (n->avl_right && query_end > n->start)
            stack[top++] = n->avl_right;

        // Prune: if addr >= n->end(), nothing in the left subtree can
        // overlap (all ends are even smaller).
        if (n->avl_left && addr < n->end())
            stack[top++] = n->avl_left;
    }

    return nullptr;
}

VirtAddr VMATree::find_free_region(VirtAddr hint,
                                    usize    size,
                                    VirtAddr limit) const noexcept {
    if (size == 0 || hint >= limit) return 0;

    VirtAddr cursor = page_align_up(hint);
    if (cursor + size > limit) return 0;

    if (!m_root) {
        return (cursor + size <= limit) ? cursor : 0;
    }

    // Collect the sorted VMA list via iterative in-order traversal 

    VMArea* stack[64];
    int     top  = 0;
    VMArea* curr = m_root;

    // Descend to the leftmost node that could produce a useful gap.
    while (curr) {
        stack[top++] = curr;
        curr = curr->avl_left;
    }

    VMArea* prev = nullptr; // last visited VMA

    while (top > 0) {
        VMArea* n = stack[--top];

        // Push the right subtree for later in-order visiting.
        VMArea* right = n->avl_right;
        while (right) {
            stack[top++] = right;
            right = right->avl_left;
        }


        VirtAddr gap_start;
        if (!prev) {
            gap_start = cursor;
        } else {
            gap_start = page_align_up(prev->end());
        }

        if (gap_start < cursor) gap_start = cursor;

        if (gap_start + size <= n->start && gap_start + size <= limit) {
            return gap_start;
        }

        prev = n;

        // Early exit: if n->start is already beyond limit, no later node
        // can help either.
        if (n->start >= limit) return 0;
    }

    // Check the gap after the last VMA
    if (prev) {
        VirtAddr gap_start = page_align_up(prev->end());
        if (gap_start < cursor) gap_start = cursor;
        if (gap_start + size <= limit) return gap_start;
    }

    return 0;
}

bool VMATree::validate() const noexcept {
    if (!m_root) return m_count == 0;

    VMArea* stack[64];
    int     top  = 0;
    VMArea* curr = m_root;

    usize   visited  = 0;
    VMArea* prev     = nullptr;
    bool    ok       = true;

    while (curr) { stack[top++] = curr; curr = curr->avl_left; }

    while (top > 0 && ok) {
        VMArea* n = stack[--top];

        // Push right subtree.
        VMArea* right = n->avl_right;
        while (right) { stack[top++] = right; right = right->avl_left; }

        ++visited;

        // (a) BST order and (b) no overlaps.
        if (prev) {
            if (n->start <= prev->start) { ok = false; break; }
            if (n->start <  prev->end())  { ok = false; break; } // overlap
        }

        // (c) AVL balance.
        int32_t bf = balance_factor(n);
        if (bf > 1 || bf < -1) { ok = false; break; }

        // (d) Correct height.
        int32_t expected_h = 1 + (node_height(n->avl_left) > node_height(n->avl_right)
                                    ? node_height(n->avl_left)
                                    : node_height(n->avl_right));
        if (n->avl_height != expected_h) { ok = false; break; }

        // (e) Parent pointer.
        if (n->avl_left  && n->avl_left->avl_parent  != n) { ok = false; break; }
        if (n->avl_right && n->avl_right->avl_parent != n) { ok = false; break; }

        prev = n;
    }

    // (f) Count.
    if (ok && visited != m_count) ok = false;

    return ok;
}
