; ========================================
; Copyright Ioane Baidoshvili 2025.
; Distributed under the terms of the MIT License.
; ========================================
; boot.asm
; In charge of jumping to kernel and defining GRUB multiboot header
; ========================================

; We're in protected mode already
[BITS 32]

section .multiboot2
align 8
multiboot2_header_start:
    ; Multiboot2 header
    dd 0xE85250D6                 ; Magic number (multiboot2)
    dd 0                          ; Architecture 0 = i386
    dd multiboot2_header_end - multiboot2_header_start ; Header length
    ; Checksum
    dd -(0xE85250D6 + 0 + (multiboot2_header_end - multiboot2_header_start))

    ; Framebuffer tag
    align 8
    dw 5                          ; type = framebuffer
    dw 0                          ; flags
    dd 20                         ; size of this tag
    dd 1024                       ; width
    dd 768                        ; height
    dd 32                         ; depth

    ; Required end tag
    align 8
    dw 0                          ; Type = end
    dw 0                          ; Flags
    dd 8                          ; Size

multiboot2_header_end:

section .text

global _start
extern kernel_main ; Kernel function that we're jumping to

; Entry point
_start:
    ; Clear interrupts
    cli
    
    ; Set up the stack
    mov esp, stack_top
    
    push ebx ; Multiboot2 info structure pointer
    push eax ; Magic number
    
    ; Call the kernel
    call kernel_main
    
    cli
.halt_loop:
    hlt
    jmp .halt_loop

; Stack allocation
section .bss
align 16
stack_bottom:
    ; 16 KB stack
    resb 16384
stack_top:
