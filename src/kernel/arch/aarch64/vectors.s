.global default_handler
.global vectors

.section .text

.macro vector
    .align 7
    b default_handler
.endm

default_handler:
    wfi
    b default_handler

.align 11
vectors:
    vector
    vector
    vector
    vector

    vector
    vector
    vector
    vector

    vector
    vector
    vector
    vector

    vector
    vector
    vector
    vector
