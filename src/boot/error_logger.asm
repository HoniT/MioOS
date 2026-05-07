; ========================================
; Copyright Ioane Baidoshvili 2026.
; Distributed under the terms of the MIT License.
;
; Simple VGA text mode error logger
; ========================================

section .boot.text
[bits 32]

global log_error

; Input: ESI must point to a null-terminated string
log_error:
    mov edi, 0xB8000
    mov ah, 0x0F

.print_loop:
    mov al, [esi]
    
    ; Check if it is the null terminator
    cmp al, 0
    je .hang

    mov word [edi], ax
    inc esi
    add edi, 2
    jmp .print_loop

.hang:
    cli
    hlt
    jmp .hang