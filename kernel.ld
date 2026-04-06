/* kernel.ld — ARMv7a bare-metal linker script
 *
 * Adjust ORIGIN/LENGTH to match your board's physical RAM layout.
 * QEMU virt defaults shown (entry at 0x40000000).
 *
 * Section layout (physical):
 *
 *   0x40000000   .text        code + rodata
 *                .data        initialised data   (copied from LMA by startup.S)
 *                .bss         zero-init data      (zeroed by startup.S)
 *                .tt          translation tables  (16 KB aligned)
 *                             ← _end lives here
 *                [bitmap]     placed by mm_init()
 *                [kheap]      placed by mm_init()
 *                [free pages] everything else
 */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

/* ── Board memory map ───────────────────────────────────────────────────── */
MEMORY {
    RAM (rwx) : ORIGIN = 0x40000000, LENGTH = 256M
}

/* ── Sections ───────────────────────────────────────────────────────────── */
SECTIONS {

    . = ORIGIN(RAM);

    /* ------------------------------------------------------------------ */
    /* .text — code                                                        */
    /* ------------------------------------------------------------------ */
    .text : {
        KEEP(*(.text.boot))   /* startup.S entry must come first           */
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } > RAM

    /* ------------------------------------------------------------------ */
    /* .data — initialised variables                                       */
    /* ------------------------------------------------------------------ */
    .data : {
        *(.data .data.*)
        . = ALIGN(4);
    } > RAM

    /* ------------------------------------------------------------------ */
    /* .bss — zero-initialised variables                                   */
    /* ------------------------------------------------------------------ */
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM

    /* ------------------------------------------------------------------ */
    /* .tt — ARMv7 short-descriptor translation tables                     */
    /*                                                                     */
    /* Must be 16 KB aligned (L1 translation table requirement).           */
    /* Marked NOLOAD: no LMA data, the MMU init code writes these.         */
    /* ------------------------------------------------------------------ */
    .tt (NOLOAD) : {
        . = ALIGN(0x4000);             /* 16 KB — L1 table requirement    */
        __tt_start = .;
        KEEP(*(.tt))
        . = ALIGN(4);
        __tt_end = .;
    } > RAM

    /* ------------------------------------------------------------------ */
    /* Kernel image boundary                                               */
    /* mm_init() uses _end as the first free physical address.             */
    /* ------------------------------------------------------------------ */
    . = ALIGN(4096);    /* page-align so bitmap placement is clean         */
    _end = .;

    /* ------------------------------------------------------------------ */
    /* Debug / symbol info — not loaded                                    */
    /* ------------------------------------------------------------------ */
    .ARM.exidx   : { *(.ARM.exidx*) }
    .ARM.extab   : { *(.ARM.extab*) }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
    }
}

/* Convenience symbols for startup.S BSS zeroing */
PROVIDE(__bss_start__ = __bss_start);
PROVIDE(__bss_end__   = __bss_end);

/* Expose table addresses for the MMU init code if needed */
PROVIDE(__tt_base = __tt_start);
