.global context_switch

.section .text, "ax", %progbits
.type context_switch, %function

.equ PROC_CTX,       16   // offsetof(struct process, ctx)
.equ CTX_SP,         52   // offsetof(cpu_ctx_t, sp)
.equ CTX_LR,         56   // offsetof(cpu_ctx_t, lr)
.equ CTX_PC,         60   // offsetof(cpu_ctx_t, pc)
.equ CTX_CPSR,       64   // offsetof(cpu_ctx_t, cpsr)
.equ PROC_SUSPENDED, 84   // offsetof(struct process, suspended)
.equ PROC_PGD ,       4    // offsetof(struct process, pgd)

context_switch:
    // r0 = next (process_t*)

    cpsid i                      // disable IRQs during switch

    // current_proc = next
    ldr   r1, =current_proc
    str   r0, [r1]

    // Switch address space
    mov   r2, r0                         // r2 = next (save across bl)
    ldr   r0, [r2, #PROC_PGD]           // r0 = next->pgd
    bl    mmu_set_proc_table

    // r1 = &next->ctx
    add   r1, r2, #PROC_CTX

    // Restore CPSR into SPSR (for exception return)
    ldr   r2, [r1, #CTX_CPSR]
    msr   spsr_cxsf, r2

    // Load PC into LR (SVC mode)
    ldr   lr, [r1, #CTX_PC]

    // Restore user SP/LR via System mode
    cps   #31                     // System mode
    ldr   sp, [r1, #CTX_SP]
    ldr   lr, [r1, #CTX_LR]
    cps   #19

    // Restore r0–r12
    ldm   r1, {r0-r12}

    // Return to user mode
    movs  pc, lr                // CPSR ← SPSR, PC ← LR
