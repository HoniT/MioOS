; ========================================
; Copyright Ioane Baidoshvili 2025.
; Distributed under the terms of the MIT License.
; ========================================
; enable_paging.asm
; In charge of paging enable/set functions that require ASM
; ========================================

[BITS 32] ; In protected mode

section .text

; Global symbols
global set_pdpt
global enable_pae
global enable_paging
global flush_tlb

; Sets the Page Directory Pointer Table in CR3
set_pdpt:
    ; Retrieving PDPT from stack
    mov eax, [esp + 4]
    ; Loading PDPT in CR3
    mov cr3, eax
    ret

; Enables Physical Address Extension
enable_pae:
    cli 
    mov eax, cr4
    or eax, 0x20 ; Set bit 5 (PAE bit)
    mov cr4, eax
    sti
    ret

enable_paging:
    cli
    mov eax, cr0
    or eax, 0x80000000 ; Set bit 31 (PG bit)
    mov cr0, eax
    sti
    ret

; Invalidate a Single Page (TLB Entry)
flush_tlb:
    ; Retrieving page from stack
    mov eax, [esp + 4]
    invlpg [eax]
    ret
