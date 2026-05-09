#include "kernel/arch/aarch64/cpu.h"

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

.macro sync, handler
    sub sp, sp, #FRAME_SIZE

    // general pupose registers
    stp x0,  x1,  [sp, #0]
    stp x2,  x3,  [sp, #16]
    stp x4,  x5,  [sp, #32]
    stp x6,  x7,  [sp, #48]
    stp x8,  x9,  [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30,      [sp, #240]

    // special registers
    mrs x0, sp_el0
    mrs x1, elr_el1
    mrs x2, spsr_el1
    str x0, [sp, #248]
    str x1, [sp, #256]
    str x2, [sp, #264]

    mov x0, sp
    bl \handler

    ldr x2, [sp, #264]
    ldr x1, [sp, #256]
    ldr x0, [sp, #248]
    msr spsr_el1, x2
    msr elr_el1, x1
    msr sp_el0, x0

    ldp x0,  x1,  [sp, #0]
    ldp x2,  x3,  [sp, #16]
    ldp x4,  x5,  [sp, #32]
    ldp x6,  x7,  [sp, #48]
    ldp x8,  x9,  [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldr x30,      [sp, #240]
    add sp, sp, #FRAME_SIZE
    eret
.endm

el1_sync:
    sync el1_sync_handler

el0_sync:
    sync el0_sync_handler

default_handler:
    wfi
    b default_handler

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

    vector el0_sync
    default_vector
    default_vector
    default_vector

    default_vector
    default_vector
    default_vector
    default_vector
