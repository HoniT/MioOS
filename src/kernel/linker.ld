ENTRY(_start)

KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS
{
    /* GRUB loads the kernel at 1MB physical */
    . = 1M;
    kernel_start_phys = .;

    .boot : {
        KEEP(*(.multiboot_header))
        *(.boot.text)
        *(.boot.data)
        *(.boot.rodata)
        *(.boot.bss)
    }

    /* Moving to HHK */
    . += KERNEL_VMA;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA) {
        _text_start = .;
        *(.text)
        _text_end = .;
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA) {
        _rodata_start = .;
        *(.rodata)
        _rodata_end = .;
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA) {
        _data_start = .;
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA) {
        *(COMMON)
        *(.bss)
        _bss_end = .;
    }

    kernel_end_phys = . - KERNEL_VMA;
}