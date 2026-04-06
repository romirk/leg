/*
 * Virtual memory layout (after MMU is enabled):
 *
 *   0x00000000  ROM identity map (unmapped after kernel copy)
 *   0x08000000  GIC identity map
 *   0x09000000  UART identity map
 *   0x40000000  DTB + early heap identity map (2 sections, from DTB pointer)
 *       ...     unmapped (future user space, 3GB)
 *   0xC0000000  kernel image (1MB):
 *                 .vtbl .text .rodata .data | .tt (NOLOAD) | .bss
 *                 STACK_BOTTOM = 0xC0100000 (stack grows downward)
 *
 * Physical memory layout (example: QEMU virt, DTB at 0x40000000):
 *
 *   0x00000000  ROM — boot vtable, .startup code, kernel load image (AT>bin)
 *   0x40000000  DTB blob (up to 1MB, padded by QEMU)
 *   0x40100000  early bump heap (128KB)
 *   0x40200000  kernel_phys_base — kernel image copied here by kboot
 *   0x40300000  free RAM (managed by page allocator after mm_init)
 *
 * The kernel virtual address (0xC0000000) is fixed. The physical placement
 * is computed at boot: kernel_phys_base = (dtb_base & ~0xFFFFF) + 0x200000.
 * No compile-time dependency on RAM address or size.
 */

ENTRY(_start)

MEMORY {
    low  (rx) : o = 0x00000000, l = 0x00100000
    krn       : o = 0xC0000000, l = 0x00100000
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
    .bss         : { bss_beg = .; *( .bss  .bss.* ) bss_end = .; } >krn AT>bin

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