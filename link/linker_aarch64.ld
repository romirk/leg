ENTRY(_start)

KERNEL_PA = 0x40000000;
KERNEL_VA = 0xFFFFFFFFC0000000;

MEMORY {
    phy       : o = KERNEL_PA , l = 0x40000000
    vir       : o = KERNEL_VA , l = 0x3FFFFFFF
}

SECTIONS {
    __kernel_load_beg = LOADADDR(.text);
    .text   : ALIGN(0x1000) { *( .text   .text.*   ) } > vir AT > phy
    .rodata : ALIGN(0x1000) { *( .rodata .rodata.* ) } > vir AT > phy
    .data   : ALIGN(0x1000) { *( .data   .data.*   ) } > vir AT > phy
    __kernel_load_end = LOADADDR(.data) + SIZEOF(.data);
    .bss    : ALIGN(0x1000) {
        __bss_start = .;
        *( .bss  .bss.* )
        __bss_end = .;
    } > vir
}
