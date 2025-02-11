.section BOOT,"ax"
        .global entry
        .type entry, "function"

entry:
        MRS  x0, MPIDR_EL1
        AND  x0, x0, #0xffff 
        CBZ  x0, boot

sleep:
        WFI
        B    sleep

boot:
        MSR  CPTR_EL3, xzr

        .global __main
        B       __main

