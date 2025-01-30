; ========================================
; Copyright Ioane Baidoshvili 2025.
; Distributed under the terms of the MIT License.
; ========================================
; gdt_flush.asm
; In charge of flushing the GDT and TSS
; ========================================

; We're in protected mode
[BITS 32]

section .text

global gdt_flush
global tss_flush

gdt_flush:
    ; Loading GDT (we're getting the gdt pointer from cpp, hence esp + 4)
    mov eax, [esp + 4]
    lgdt [eax]

    ; Setting segments
    mov ax, 0x10 ; Data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush ; Far jump using Code Segment

    .flush:
        ret ; Returning

; Legacy TSS flush, before userland
tss_flush:
    mov ax, 0x28 ; TSS segment in GDT (6th segment)
    ltr ax

    ret
    