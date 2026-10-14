KERNEL_PA   = 0x0000000040200000;
PAGE_OFFSET = 0xFFFFFF8000000000;

_start_phy  = ABSOLUTE(_start - PAGE_OFFSET);
ENTRY(_start_phy)

SECTIONS {
    . = KERNEL_PA + PAGE_OFFSET;
    .text   : AT(ADDR(.text) - PAGE_OFFSET) ALIGN(0x1000)
                            { *( .text   .text.*   ) }
    .rodata : ALIGN(0x1000) { *( .rodata .rodata.* ) }
    .data   : ALIGN(0x1000) { *( .data   .data.*   ) }

    .tt     : ALIGN(0x1000) { *( .tt     .tt.*     ) }
    .bss    : ALIGN(0x1000) {
        __bss_start = .;
        *( .bss  .bss.* )
        __bss_end = .;
    }

    __kernel_load_beg = LOADADDR(.text);
    __kernel_load_end = LOADADDR(.tt) + SIZEOF(.tt);
    __kernel_phys_end = LOADADDR(.bss) + SIZEOF(.bss);
}

/* Must match memory.h and the direct map built in tt.c (L1[1] → 512 × 2 MB). */
RAM_BASE = 0x0000000040000000;
RAM_SIZE = 0x0000000040000000;

ASSERT(KERNEL_PA % 0x200000 == 0, "KERNEL_PA must be 2 MB aligned")
ASSERT(__kernel_phys_end <= RAM_BASE + RAM_SIZE, "kernel image (incl. .bss) runs past mapped RAM")
