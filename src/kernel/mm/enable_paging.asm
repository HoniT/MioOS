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
global set_pd
global enable_paging
global reload_cr3
global invlpg

; Sets the Page Directory in CR3
set_pd:
    cli
    ; Retrieve PD address
    mov eax, [esp + 4]
    mov cr3, eax
    sti
    ret

enable_paging:
    cli

    ; Enabling PSE
    mov eax, cr4
    xor eax, eax
    or eax, 0x10
    mov cr4, eax

    ; Enabling paging
    mov eax, cr0
    or eax, 1 << 31 ; Set bit 31 (PG bit)
    mov cr0, eax
    sti
    ret

; Invalidate TLB by reloading CR3
reload_cr3:
    ; Reloading CR3
    cli
    mov eax, cr3
    mov cr3, eax
    sti
    ret

; Invalidates a single virtual address
invlpg:
    mov eax, [esp + 4]
    invlpg [eax]
    ret