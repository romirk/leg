.global _start

.section .boot, "ax"

.equ STACK_TOP, 0x40400000
.equ UARTDR, 0x09000000

_start:
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
    ldr  x1, =STACK_TOP
    mov  sp, x1

    mov  x0, #UARTDR
    mov  w1, #'C'
    strb w1, [x0]
    mov  w1, #'\n'
    strb w1, [x0]
1:
    wfi
    b 1b
