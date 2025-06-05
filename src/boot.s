.global _start

.section .boot_vtbl, "ax", %progbits
.L.boot_vtable:
    // reset
    b handle_reset

    // undefined instruction
    ldr pc, =handle_boot_exception

    // software interrupt
    ldr pc, =handle_boot_exception

    // external/internal prefetch abort
    ldr pc, =handle_boot_exception

    // external/internal data abort
    ldr pc, =handle_boot_exception

    // no exception
    nop

    // IRQ
    ldr pc, =handle_boot_exception

    // FIQ
    ldr pc, =handle_boot_exception

.section .vtbl, "ax", %progbits
vtable:
    // reset
    ldr pc, =.L.boot_vtable

    // undefined instruction
    nop

    // supervisor call
    b handle_svc

    // external/internal prefetch abort
    nop

    // external/internal data abort
    nop

    // no exception
    nop

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
    ldr sp, = STACK_BOTTOM - 0x0c00
    cps #0b11011            // Undefined mode
    ldr sp, = STACK_BOTTOM - 0x1000
    cps #0b10011            // Supervisor (kernel) mode
    ldr sp, = STACK_BOTTOM - 0x1400

    b kboot



/*
enable_mmu:
    mrc p15, 0, r1, c1, c0, 0   // Read control register
    orr r1, #0x01               // set M bit
    mcr p15, 0, r1, c1, c0, 0   // Write control register and enable MMU
*/
