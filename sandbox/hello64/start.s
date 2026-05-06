.global _start

.section .text
_start:
    mov x0, #0x1234
    mov x1, #0x5678
.L.loop:
    wfi
    b .L.loop
