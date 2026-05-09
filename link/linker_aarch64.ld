ENTRY(_start)

KERNEL_PA = 0x40100000;
KERNEL_VA = 0xFFFFFFFFC0100000;

MEMORY {
    phy       : o = KERNEL_PA , l = 0x40000000
    vir       : o = KERNEL_VA , l = 0x3FEFFFFF
    btm       : o = 0x40180000, l = 0x00080000
}

SECTIONS {
    .text   : ALIGN(0x1000) { *( .text   .text.*   ) } > vir AT > phy
    __kernel_load_beg = LOADADDR(.text);
    .rodata : ALIGN(0x1000) { *( .rodata .rodata.* ) } > vir AT > phy
    .data   : ALIGN(0x1000) { *( .data   .data.*   ) } > vir AT > phy
    __kernel_load_end = LOADADDR(.data) + SIZEOF(.data);
    .bss    : ALIGN(0x1000) {
        __bss_start = .;
        *( .bss  .bss.* )
        __bss_end = .;
    } > vir
    .boot : ALIGN(0x1000) { *( .boot .boot.* ) } > btm AT > btm
}
