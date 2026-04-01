section .multiboot_header
align 8
header_start:
    dd 0xE85250D6                ; Magic number (Multiboot 2)
    dd 0                         ; Architecture 0 (Protected Mode i386)
    dd header_end - header_start ; Header length
    ; Checksum
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start)) 
    ; Required end tag
    dw 0, 0
    dd 8
header_end:

; Added 'nobits' to tell NASM this is uninitialized memory
section .boot.bss nobits
align 4096
p4_table: resb 4096
p3_table: resb 4096
p2_table: resb 4096
stack_bottom: resb 16384         ; 16KB stack
stack_top:

section .boot.text
global _start
bits 32
_start:
    mov esp, stack_top

    ; Set up Page Tables (Identity map + Higher half)
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

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov eax, p4_table
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt64.pointer]

    jmp gdt64.code_segment:realm64

bits 64
extern kernel_main
realm64:
    ; Clear old segment registers
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Update stack pointer to point to the higher half virtual address
    mov rax, 0xFFFFFFFF80000000
    add rsp, rax

    mov rax, kernel_main
    call rax

    ; Halt if kernel_main returns
    cli
.hlt:
    hlt
    jmp .hlt

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
    