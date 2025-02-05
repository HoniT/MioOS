; ========================================
; Copyright Ioane Baidoshvili 2025.
; Distributed under the terms of the MIT License.
; ========================================
; enable_paging.asm
; In charge of enabling paging and setting PD in CR3
; ========================================


section .text
    global enable_paging
    global set_page_directory

set_page_directory:
    ; Getting the page directory and setting it in CR3
    mov eax, [esp + 4]
    mov cr3, eax

    ret

enable_paging:
    cli

    ; Setting the paging bit
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    sti
    ret
