#include "kernel/arch/aarch64/boot.h"

.global _start

.bss
.balign 4096
boot_stack:
    .skip BOOT_STACK_SIZE
boot_stack_top:

.section .text, "ax"

_start:
    mov  x19, x0
    mrs  x2, CurrentEL
    lsr  x2, x2, #2          // bits [3:2] → [1:0]
    cmp  x2, #2
    b.ne el1_main            // already at EL1, skip the drop

    mrs  x1, hcr_el2
    orr  x1, x1, #(1 << 31)
    msr  hcr_el2, x1

    mov  x1, #0x3C5
    msr  spsr_el2, x1

    adr  x1, el1_main
    msr  elr_el2, x1

    dsb  sy
    isb
    eret

el1_main:
    adrp x1, boot_stack_top
    add  x1, x1, :lo12:boot_stack_top
    mov  sp, x1

    bl   init_mmu

    ldr  x0, =PAGE_OFFSET_
    ldr  x1, =kentry

    // relocate stack pointer
    add  sp, sp, x0
    // jump
    br   x1

tlb_flush:
    dsb  sy
    tlbi vmalle1
    dsb  nsh
    isb
    ret

kentry:
    // install vtable
    adr  x1, vectors
    msr  vbar_el1, x1

    // prevent ttbr0 walks
    mrs  x0, tcr_el1
    orr  x0, x0, #(1 << 7)
    msr  tcr_el1, x0
    isb

    bl   tlb_flush

    // call kernel and pass device tree
    mov  x0, x19
    b    kmain
