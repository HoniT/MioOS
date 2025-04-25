// ========================================
// Copyright Ioane Baidoshvili 2025.
// Distributed under the terms of the MIT License.
// ========================================
// gdt.cpp
// In charge of setting up the Global Descriptor Table
// ========================================

#include <drivers/vga_print.hpp>
#include <gdt.hpp>
#include <lib/mem_util.hpp>

// Arrays and variables
__attribute__((aligned(8))) gdt_entry gdt_entries[GDT_SEGMENT_QUANTITY];
gdt_ptr _gdt_ptr;
tss_entry _tss_entry;

void gdt::init(void) {
    // Set up GDT pointer
    _gdt_ptr.limit = (sizeof(struct gdt_entry) * GDT_SEGMENT_QUANTITY) - 1;
    _gdt_ptr.base = uint32_t(&gdt_entries); // Address of gdt_entries[0]

    // Setting up GDT registers
    set_gdt_gate(0, 0, 0, 0, 0); //Null segment
    set_gdt_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel code segment
    set_gdt_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel data segment
    set_gdt_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User code segment
    set_gdt_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User data segment
    // write_tss(5, 0x10, 0x0); // TSS

    // Flushing GDT and TSS
    gdt_flush((uint32_t)&_gdt_ptr);
    vga::printf("Implemented GDT at %x!\n", _gdt_ptr.base);

    // tss_flush();
    // vga::printf("Implemented TSS!\n");
}

void gdt::set_gdt_gate(const uint32_t num, const uint32_t base, const uint32_t limit, const uint8_t access, const uint8_t gran) {
    // Setting up bases
    gdt_entries[num].base_low = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    // Flags, limit and access flags
    gdt_entries[num].limit = (limit & 0xFFFF);
    gdt_entries[num].flags = (limit >> 16) & 0x0F;
    gdt_entries[num].flags |= (gran & 0xF0); // Granuality
    gdt_entries[num].access = access;

}

void gdt::write_tss(const uint32_t num, const uint16_t ss0, const uint32_t esp0) {
    uint32_t base = (uint32_t)&_tss_entry; // TSS address
    uint32_t limit = base + sizeof(_tss_entry);

    // Setting in GDT
    gdt::set_gdt_gate(num, base, limit, 0xE9, 0x00);
    memset(&_tss_entry, 0, sizeof(_tss_entry));

    // Setting TSS flags/properties
    _tss_entry.ss0 = ss0;
    _tss_entry.esp0 = esp0;
    _tss_entry.cs = 0x08 | 0x3;
    _tss_entry.ss = _tss_entry.ds = _tss_entry.es = _tss_entry.fs = _tss_entry.gs = 0x10 | 0x3;
}
