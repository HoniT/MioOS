; ========================================
; Copyright Ioane Baidoshvili 2025.
; Distributed under the terms of the MIT License.
; ========================================
; idt_def.asm
; In charge of defining ISRs and IRQs with their common stubs
; ========================================

; We're in protected mode
[BITS 32]

section .text
global idt_flush

; Flushes the IDT
idt_flush:
    mov eax, [esp + 4] ; Getting parameter from cpp
    lidt [eax] ; Loads IDT

    sti ; Enables interrupts
    ret


; Macros

%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        cli
        push long 0
        push long %1
        jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        cli
        push long %1
        jmp isr_common_stub
%endmacro 

%macro IRQ 2
    global irq%1
    irq%1:
        cli
        push long 0
        push long %2
        jmp irq_common_stub
%endmacro

; ====================
; Error codes
; ====================

    ISR_NOERRCODE 0
    ISR_NOERRCODE 1
    ISR_NOERRCODE 2
    ISR_NOERRCODE 3
    ISR_NOERRCODE 4
    ISR_NOERRCODE 5
    ISR_NOERRCODE 6
    ISR_NOERRCODE 7

    ISR_ERRCODE 8
    ISR_NOERRCODE 9 
    ISR_ERRCODE 10
    ISR_ERRCODE 11
    ISR_ERRCODE 12
    ISR_ERRCODE 13
    ISR_ERRCODE 14
    ISR_NOERRCODE 15
    ISR_NOERRCODE 16
    ISR_NOERRCODE 17
    ISR_NOERRCODE 18
    ISR_NOERRCODE 19
    ISR_NOERRCODE 20
    ISR_NOERRCODE 21
    ISR_NOERRCODE 22
    ISR_NOERRCODE 23
    ISR_NOERRCODE 24
    ISR_NOERRCODE 25
    ISR_NOERRCODE 26
    ISR_NOERRCODE 27
    ISR_NOERRCODE 28
    ISR_NOERRCODE 29
    ISR_NOERRCODE 30
    ISR_NOERRCODE 31
    
    ; Syscalls
    ISR_NOERRCODE 128
    ISR_NOERRCODE 177


    IRQ 0, 32
    IRQ 1, 33
    IRQ 2, 34
    IRQ 3, 35
    IRQ 4, 36
    IRQ 5, 37
    IRQ 6, 38
    IRQ 7, 39
    IRQ 8, 40
    IRQ 9, 41
    IRQ 10, 42
    IRQ 11, 43
    IRQ 12, 44
    IRQ 13, 45
    IRQ 14, 46
    IRQ 15, 47


; Hanldelers

extern isr_handler

; ISR common stub
isr_common_stub:
    pusha                    ; Push all registers

    mov eax, ds
    push eax                 ; Save DS segment

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                 ; Pass pointer to struct (InterruptRegisters*)
    call isr_handler

    add esp, 4               ; Remove pushed values (CR2 + DS)
    pop ebx

    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa
    sti                      ; Re-enable interrupts
    iret

extern irq_handler

; IRQ common stub
irq_common_stub:
    pusha

    ; Stack
    mov eax, ds
    push eax
    mov eax, cr2
    push eax 

    ; Setting up segments
    mov ax, 0x10 
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler


    add esp, 8
    pop ebx

    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa
    add esp, 8

    sti ; Enables interrupts
    iret 
