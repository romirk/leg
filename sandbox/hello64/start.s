.global _start

.section .text

.equ UARTDR, 0x09000000
.macro putc, c
    mov x1, \c
    strb w1, [x0]
.endm

print_el:
    mov  x0, #UARTDR
    mrs  x1, CurrentEL
    and  x1, x1, #0b1100
    lsr  x1, x1, #2
    add  x1, x1, #'0'
    putc x1
    putc #'\n'
    ret

_start:
    bl print_el   

    mrs  x1, hcr_el2
    orr  x1, x1, #(1 << 31)
    msr  hcr_el2, x1

    mov  x1, #0x3C5
    msr  spsr_el2, x1  

    adr x1, el1_main
    msr elr_el2, x1

    dsb sy
    isb sy
    eret

el1_main:
    bl print_el

.L.loop:
    wfi
    b .L.loop
