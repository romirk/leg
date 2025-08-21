.global _start
.global vtable

.section .boot_vtbl, "ax", %progbits
_start:
.L.boot_vtable:
    // reset
    b handle_reset

    // undefined instruction
    b handle_boot_exception

    // software interrupt
    b handle_boot_exception

    // external/internal prefetch abort
    b handle_boot_exception

    // external/internal data abort
    b handle_boot_exception

    // no exception
    b .

    // IRQ
    b handle_boot_exception

    // FIQ
    b handle_boot_exception

.section .vtbl, "ax", %progbits
vtable:
    // reset
    mov pc, #0

    // undefined instruction
    b .

    // supervisor call
    b handle_svc

    // external/internal prefetch abort
    b handle_data_abort

    // external/internal data abort
    b handle_data_abort

    // no exception
    b .

    // IRQ
    b handle_irq

    // FIQ
    b handle_fiq

.size vtable, . - vtable

.section .startup, "ax", %progbits
handle_reset:
    // init stacks
    cps #0b10001            // FIQ mode
    ldr sp, = STACK_BOTTOM - 0x0000
    cps #0b10010            // IRQ mode
    ldr sp, = STACK_BOTTOM - 0x0400
    cps #0b10111            // Abort mode
    ldr sp, = STACK_BOTTOM - 0x0800
    cps #0b11011            // Undefined mode
    ldr sp, = STACK_BOTTOM - 0x0c00
    cps #0b10011            // Supervisor (kernel) mode
    ldr sp, = 0x40100000 - 0x1000

    // from arm docs
    // Enable access to CP10 and CP11 and clear the ASEDIS bit in the CPACR
    mov r0, 0x00f00000
    mcr p15, 0, r0, c1, c0, 2
    isb

    // Set the FPEXC.EN bit to enable Advanced SIMD and VFP
    mov  r1, #0x40000000
    vmsr FPEXC, r1

    b kboot
