; ========================================
; Copyright Ioane Baidoshvili 2025.
; Distributed under the terms of the MIT License.
; ========================================
; boot.asm
; In charge of jumping to kernel and defining GRUB multiboot header
; ========================================

; We're in protected mode already
[BITS 32]

section .text

; GRUB multiboot header
align 4
dd 0x1BADB002          ; GRUB magic number
dd 0x00000003          ; Flags
dd -(0x1BADB002 + 0x3) ; Checksum

global _start
global stack_space
extern kernel_main ; Kernel function that we're jumping to

; Entry point
_start:
    cli ; Clear interrupts
    mov esp, stack_space ; Setting stack pointer

    ; Kernel main parameters
    push ebx
    push eax
    call kernel_main

    hlt
; Infinite halt loop
halt_loop:
    hlt
    jmp halt_loop

; Stack label for stack pointer
section .bss
resb 8192
stack_space:
