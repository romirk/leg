.arch  armv8.2-a

.section BOOT, "ax"
    .global _start
    .type   _start, "function"

_start:
    mrs x0, MPIDR_EL1
    and x0, x0, #0xffff
    cbz x0, boot

sleep:
    wfi
    b   sleep

boot:
    msr CPTR_EL3, xzr

    .global __main
    b __main
