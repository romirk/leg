.global default_handler
.global vectors

.section .text

.macro default_vector
    .align 7
    b default_handler
.endm

.macro vector, label
    .align 7
    b \label
.endm

default_handler:
    wfi
    b default_handler

el1_sync:
    stp x29, x30, [sp, #-16]!
    bl el1_sync_handler

.align 11
vectors:
    default_vector
    default_vector
    default_vector
    default_vector

    vector el1_sync
    default_vector
    default_vector
    default_vector

    default_vector
    default_vector
    default_vector
    default_vector

    default_vector
    default_vector
    default_vector
    default_vector
