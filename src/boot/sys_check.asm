; ========================================
; Copyright Ioane Baidoshvili 2026.
; Distributed under the terms of the MIT License.
;
; System hardware checks
; ========================================

section .boot.rodata

err_msg_cpuid:     db "CPUID not supported", 0
err_msg_ext_cpuid: db "Extended CPUID not supported", 0
err_msg_long_mode: db "Long mode not supported", 0

section .boot.text
[bits 32]

global check_cpuid
global check_extended_cpuid
global check_long_mode

extern log_error

check_cpuid:
    pushfd
    pop eax

    mov ecx, eax
    ; Flip the ID bit (bit 21)
    xor eax, 1 << 21

    push eax
    popfd

    pushfd
    pop eax

    ; Restore the original EFLAGS from ECX
    push ecx
    popfd

    ; Compare EAX (what EFLAGS is now) with ECX (what EFLAGS was originally)
    cmp eax, ecx
    je .no_cpuid
    ret

.no_cpuid:
    mov esi, err_msg_cpuid
    jmp log_error

check_extended_cpuid:
    ; Set EAX to 0x80000000 to ask for the highest extended function
    mov eax, 0x80000000
    cpuid

    ; Check if the maximum extended function is at least 0x80000001
    cmp eax, 0x80000001
    jb .no_extended_cpuid
    ret

.no_extended_cpuid:
    mov esi, err_msg_ext_cpuid
    jmp log_error

check_long_mode:
    ; Ask for extended processor info
    mov eax, 0x80000001
    cpuid

    ; Test if bit 29 in EDX is set (the LM bit)
    test edx, 1 << 29
    jz .no_long_mode
    ret

.no_long_mode:
    ; Load i386 instead of x86_64 in the future (if i'll support multiple archs)
    mov esi, err_msg_long_mode
    jmp log_error