.global _start

.section .boot_itbl, "ax", %progbits
vtable:
    // reset
    b handle_reset

    // undefined instruction
    ldr pc, =handle_boot_exception

    // software interrupt
    ldr pc, =handle_boot_exception

    // external/internal prefetch abort
    ldr pc, =handle_boot_exception

    // external/internal data abort
    ldr pc, =handle_page_fault_boot

    // no exception
    nop

    // IRQ
    ldr pc, =handle_irq_boot

    // FIQ
    ldr pc, =handle_fiq_boot

.size vtable, . - vtable

.section .startup, "ax", %progbits
handle_reset:
    // init stacks
    cps #0b10001            // FIQ mode
    ldr sp, =0x40100000
    cps #0b10010            // IRQ mode
    ldr sp, =0x400f0000
    cps #0b10111            // Abort mode
    ldr sp, =0x400e0000
    cps #0b11011            // Undefined mode
    ldr sp, =0x400d0000
    cps #0b10011            // Supervisor (kernel) mode
    ldr sp, =STACK_BOTTOM

    b kboot



/*
enable_mmu:
    mrc p15, 0, r1, c1, c0, 0   // Read control register
    orr r1, #0x01               // set M bit
    mcr p15, 0, r1, c1, c0, 0   // Write control register and enable MMU
*/
