
/* Kernel linker file */
OUTPUT_FORMAT(elf32-i386)
ENTRY(_start) /* Entry in boot.asm */

SECTIONS {
    . = 0x100000; /* The kernel starts at 1MiB in physical memory */

    .text : AT(ADDR(.text)) ALIGN(4096) 
    {
        *(.text*)
    }
    
    .rodata : AT(ADDR(.rodata)) ALIGN(4096)
    {
        *(.rodata*)
    }

    .data : AT(ADDR(.data)) ALIGN(4096)
    {
        *(.data*)
    }

    .bss : AT(ADDR(.bss)) ALIGN(4096)
    {
        *(.bss*)
    }
}
