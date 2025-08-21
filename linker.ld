ENTRY(_start)

MEMORY {
    low  (rx) : o = 0x00000000, l = 0x00100000
    ram       : o = 0x80000000, l = 0x40000000
    krn       : o = 0xbff00000, l = 0x00100000
    bin       : o = 0x00000000, l = 0x00100000
}

OUTPUT_ARCH(scriptfile)
SECTIONS {
    .boot_vtbl : { *( .boot_vtbl             ) } >low AT>bin
    .startup   : { *( .startup    .startup.* ) } >low AT>bin
    .startup.end : ALIGN(0x10) {} >low AT>bin

    kernel_load_beg = LOADADDR(.startup.end);
    kernel_main_beg = ORIGIN(krn);

    .vtbl   : { *( .vtbl   .vtbl.*   ) } >krn AT>bin
    .text   : { *( .text   .text.*   ) } >krn AT>bin
    .rodata : { *( .rodata .rodata.* ) } >krn AT>bin
    .data   : { *( .data   .data.*   ) } >krn AT>bin

    kernel_main_end = .;

    .tt (NOLOAD) : { *( .tt   .tt.*  ) } >krn
    .bss         : { *( .bss  .bss.* ) } >krn AT>bin

    STACK_BOTTOM = ORIGIN(krn) + LENGTH(krn);

    /**
     * C compilers for ARM generate a special section containing exception
     * handling information, and there is no easy way to disable it in Clang.
     * As such, we're forced to explicitly discard it here (if we don't mention
     * it here, it gets stored at the beginning of the binary, forcing the IVT
     * out of the way).
     */
    /DISCARD/ : { *( .ARM.exidx .ARM.exidx.* ) }
}