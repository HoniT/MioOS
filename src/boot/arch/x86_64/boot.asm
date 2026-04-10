; ========================================
; Copyright Ioane Baidoshvili 2026.
; Distributed under the terms of the MIT License.
; ========================================

; ==== GRUB HEADERS ====
section .multiboot_header
align 8
header_start:
    dd 0xE85250D6
    dd 0 ; Architecture 0 (i386)
    dd header_end - header_start
    ; Checksum
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start)) 
    ; Required end tag
    dw 0, 0
    dd 8
header_end:

section .boot.bss nobits

global p4_table
align 4096
p4_table: resb 4096
p3_table: resb 4096
p2_table: resb 4096

align 16
stack_bottom: resb 16384 ; 16KB stack
stack_top:

section .boot.rodata
err_msg_multiboot: db "Invalid Multiboot2 magic", 0
err_msg_cpuid:     db "CPUID not supported", 0
err_msg_ext_cpuid: db "Extended CPUID not supported", 0
err_msg_long_mode: db "Long mode not supported", 0

; ==== EARLY BOOT ====
section .boot.text
[bits 32]

extern log_error
global _start
_start:
    cli

    mov esp, stack_top

    mov edi, ebx ; Multiboot info pointer (Physical)
    mov esi, eax ; Magic number

    ; Verifying GRUB magic
    cmp esi, 0x36D76289
    jne .no_magic

    ; Verifying CPUID
    call check_cpuid
    ; Verifying extended cpuid
    call check_extended_cpuid
    ; Verifying long mode
    call check_long_mode

    mov eax, p3_table
    or eax, 0b11 ; Present, Writable
    mov [p4_table], eax

    ; Point PML4[511] to PDP (Higher half map)
    mov [p4_table + 511 * 8], eax

    ; Point PDP[510] to PD (Maps to 0xFFFFFFFF80000000)
    mov eax, p2_table
    or eax, 0b11 ; Present, Writable
    mov [p3_table + 510 * 8], eax
    
    ; Also map PDP[0] to PD for the identity map
    mov [p3_table], eax

    ; Map the PD table using 2MB huge pages
    mov ecx, 0

.map_p2_table:
    mov eax, 0x200000 ; 2MB size
    mul ecx
    or eax, 0b10000011 ; Present, Writable, Huge Page
    mov [p2_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_p2_table

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Load PML4
    mov eax, p4_table
    mov cr3, eax

    ; Enable Long Mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable Paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; ENABLE SSE
    mov eax, cr0
    and ax, 0xFFFB
    or ax, 0x2
    mov cr0, eax
    mov eax, cr4
    or ax, 3 << 9
    mov cr4, eax

    ; Long mode
    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:realm64

.no_magic:
    mov esi, err_msg_multiboot
    jmp log_error


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
    ; Load i386 instead of x86_64 in the future
    mov esi, err_msg_long_mode
    jmp log_error

; ==== 64 BIT REALM ====
[bits 64]
extern entry_x86_64
realm64:
    ; Clear old segment registers
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; HIGHER HALF SHIFT & KERNEL HANDOFF
    ; Shift stack and Multiboot pointers into the higher half
    mov rax, 0xFFFFFFFF80000000
    add rsp, rax 
    add rdi, rax 

    mov rax, entry_x86_64
    call rax

    cli
.hang
    cli
    hlt
    jmp .hang

section .boot.data
align 8
gdt64:
    dq 0 ; Null descriptor
.code_segment: equ $ - gdt64
    ; Executable | Descriptor | Present | 64-bit flag
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) 
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
