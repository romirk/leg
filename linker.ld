ENTRY(_start)

MEMORY {
    low  : o = 0x00000000, l = 0x00040000
    ram  : o = 0x40000000, l = 0x80000000
    dtbm : o = 0x40000000, l = 0x00100000
    boot : o = 0x40100000, l = 0x00040000
    bin  : o = 0x00000000, l = 0x00010000
}

OUTPUT_ARCH(scriptfile)
SECTIONS {
    .boot_itbl : {
        boot_itbl_beg = . ;
        *( .boot_itbl )
        boot_itbl_end = . ;
    } >low AT>bin

    .startup : { *( .startup    .startup.* ) } >low AT>bin
    .startup.end : {} >low AT>bin

    kernel_main_off = 0x40100000 + .;
    kernel_load_off = LOADADDR(.startup.end);

    .main.beg kernel_main_off (NOLOAD) : {} >boot

    .text   : { *( .text   .text.*   ) } >boot AT>bin
    .rodata : { *( .rodata .rodata.* ) } >boot AT>bin
    .data   : { *( .data   .data.*   ) } >boot AT>bin
    .bss    : { *( .bss    .bss.*    ) } >boot AT>bin

    kernel_main_end = .;
    STACK_BOTTOM = LENGTH(ram) + ORIGIN(ram);

    /**
     * C compilers for ARM generate a special section containing exception
     * handling information, and there is no easy way to disable it in Clang.
     * As such, we're forced to explicitly discard it here (if we don't mention
     * it here, it gets stored at the beginning of the binary, forcing the IVT
     * out of the way).
     */
    /DISCARD/ : { *( .ARM.exidx .ARM.exidx.* ) }
}