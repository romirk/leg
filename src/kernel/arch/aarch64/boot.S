#include "kernel/arch/aarch64/cpu.h"

.global _start

.section .text, "ax"

_start:
    mov x19, x0
    mrs  x2, CurrentEL
    lsr  x2, x2, #2          // bits [3:2] → [1:0]
    cmp  x2, #2
    b.ne el1_main            // already at EL1, skip the drop

    mrs  x1, hcr_el2
    orr  x1, x1, #(1 << 31)
    msr  hcr_el2, x1

    mov  x1, #0x3C5
    msr  spsr_el2, x1

    adr x1, el1_main
    msr elr_el2, x1

    dsb sy
    isb
    eret

el1_main:
    adrp x1, BOOT_STACK
    add  x1, x1, #(BOOT_STACK_SIZE)
    mov  sp, x1

    mov x0, x19
    bl  kboot

1:
    wfi
    b 1b
