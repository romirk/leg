.global _start

.section .text

.equ UARTDR, 0x09000000
.macro putc, c
    mov x1, \c
    strb w1, [x0]
.endm

_start:
    mov  x0, #UARTDR
    putc #'H'
    putc #'i'
    putc #'\n'
.L.loop:
    wfi
    b .L.loop
