/*
 * User-space binary linker script.
 *
 * The binary is loaded at PROC_CODE_VA (0x00100000) by the kernel at process
 * creation time.  It must fit within PROC_CODE_MAX = 1 MB.
 *
 * _user_entry is placed first in .text so it lands at exactly 0x00100000,
 * which is the entry point used by process_exec.
 */

ENTRY(_user_entry)

MEMORY {
    usr : ORIGIN = 0x00100000, LENGTH = 0x00100000
}

SECTIONS {
    .text : {
        *(.text.entry)          /* _user_entry first */
        *(.text .text.*)
    } >usr

    .rodata : { *(.rodata .rodata.*) } >usr
    .data   : { *(.data   .data.*  ) } >usr
    .bss    : { *(.bss    .bss.*   ) *(COMMON) } >usr

    /DISCARD/ : { *(.ARM.exidx .ARM.exidx.*) }
}