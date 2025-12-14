; ========================================
; Copyright Ioane Baidoshvili 2025.
; Distributed under the terms of the MIT License.
; ========================================
; context_switch.asm
; In charge of switching contexts
; ========================================

[BITS 32]

SECTION .text
global ctx_switch

; Offsets in cpu_context_t structure (must match task.hpp)
CONTEXT_EAX    equ 0
CONTEXT_EBX    equ 4
CONTEXT_ECX    equ 8
CONTEXT_EDX    equ 12
CONTEXT_ESI    equ 16
CONTEXT_EDI    equ 20
CONTEXT_EBP    equ 24
CONTEXT_ESP    equ 28
CONTEXT_EIP    equ 32
CONTEXT_EFLAGS equ 36
CONTEXT_CS     equ 40
CONTEXT_DS     equ 44
CONTEXT_ES     equ 48
CONTEXT_FS     equ 52
CONTEXT_GS     equ 56
CONTEXT_SS     equ 60
CONTEXT_CR3    equ 64

; void ctx_switch(context_t* old_ctx, context_t* new_ctx)
ctx_switch:
    ; Save current context
    ; Parameters: old_ctx in [ebp+8], new_ctx in [ebp+12]
    
    push ebp
    mov ebp, esp
    pushf                       ; Save flags
    
    mov eax, [ebp+8]            ; eax = old context pointer
    test eax, eax               ; Check if old_ctx is NULL
    jz .load_next               ; Skip saving if NULL
    
    ; Save all general purpose registers
    mov [eax + CONTEXT_EBX], ebx
    mov [eax + CONTEXT_ECX], ecx
    mov [eax + CONTEXT_EDX], edx
    mov [eax + CONTEXT_ESI], esi
    mov [eax + CONTEXT_EDI], edi
    
    ; Save EBP (before we used it)
    mov ebx, [ebp]
    mov [eax + CONTEXT_EBP], ebx
    
    ; Save ESP 
    ; We want ESP to point to the first argument (old_ctx) upon return,
    ; so the caller's 'add esp, 8' cleanup works correctly.
    ; Stack: [Arg2] [Arg1] [RetAddr] [OldEBP] [Flags]
    ; EBP points to OldEBP. Arg1 is at EBP+8.
    lea ebx, [ebp + 8]      
    mov [eax + CONTEXT_ESP], ebx
    
    ; Save EIP (return address)
    mov ebx, [ebp + 4]
    mov [eax + CONTEXT_EIP], ebx
    
    ; Save EFLAGS
    mov ebx, [ebp - 4]
    mov [eax + CONTEXT_EFLAGS], ebx
    
    ; Save segment registers (Zero extend to be safe)
    xor ebx, ebx
    mov bx, ds
    mov [eax + CONTEXT_DS], ebx
    mov bx, es
    mov [eax + CONTEXT_ES], ebx
    mov bx, fs
    mov [eax + CONTEXT_FS], ebx
    mov bx, gs
    mov [eax + CONTEXT_GS], ebx
    mov bx, ss
    mov [eax + CONTEXT_SS], ebx
    mov bx, cs
    mov [eax + CONTEXT_CS], ebx
    
    ; Save CR3 (page directory)
    mov ebx, cr3
    mov [eax + CONTEXT_CR3], ebx

.load_next:
    ; Load next task context
    mov eax, [ebp + 12]     ; eax = new context pointer
    
    ; Switch page directory if different
    mov ebx, [eax + CONTEXT_CR3]
    mov ecx, cr3
    cmp ebx, ecx
    je .same_address_space
    mov cr3, ebx
    
.same_address_space:
    ; Load segment registers
    mov bx, [eax + CONTEXT_DS]
    mov ds, bx
    mov es, bx
    mov bx, [eax + CONTEXT_FS]
    mov fs, bx
    mov bx, [eax + CONTEXT_GS]
    mov gs, bx
    
    ; Load general purpose registers
    mov ebx, [eax + CONTEXT_EBX]
    mov ecx, [eax + CONTEXT_ECX]
    mov edx, [eax + CONTEXT_EDX]
    mov esi, [eax + CONTEXT_ESI]
    mov edi, [eax + CONTEXT_EDI]
    mov ebp, [eax + CONTEXT_EBP]
    
    ; Load stack pointer
    mov esp, [eax + CONTEXT_ESP]
    
    ; Push EFLAGS, CS, and EIP for iret
    ; This overwrites the stack below ESP, which is fine as we calculated ESP to be above this data.
    push dword [eax + CONTEXT_EFLAGS]
    push dword [eax + CONTEXT_CS]
    push dword [eax + CONTEXT_EIP]
    
    ; Load EAX last
    mov eax, [eax + CONTEXT_EAX]
    
    ; Return to new task
    iretd