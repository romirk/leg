.section .text, "ax", %progbits

// ── IRQ trampoline ──────────────────────────────────────────────────────────────
//
// Handles two cases:
//   1. Interrupted user mode (spsr.M == 0x10): save full cpu_ctx_t into
//      current_proc->ctx, call irq_dispatch(), then check current_proc->suspended.
//      If suspended, enter WFI loop in SVC mode (IRQs enabled) until cleared.
//      Restore full user context and return.
//
//   2. Interrupted kernel/SVC code: save volatile regs on IRQ stack, call
//      irq_dispatch(), restore and return.
//
// struct process memory layout (offsets used below):
//   +0   pid          +4  pgd          +8  sp (initial)
//   +12  stack_phys   +16 entry
//   +20  ctx.r[0..12] (52 bytes, ends at +72)
//   +72  ctx.sp       +76 ctx.lr       +80 ctx.pc        +84 ctx.cpsr
//   +88  suspended
//
.equ PROC_CTX,       16   // offsetof(struct process, ctx)
.equ CTX_SP,         52   // offsetof(cpu_ctx_t, sp)
.equ CTX_LR,         56   // offsetof(cpu_ctx_t, lr)
.equ CTX_PC,         60   // offsetof(cpu_ctx_t, pc)
.equ CTX_CPSR,       64   // offsetof(cpu_ctx_t, cpsr)
.equ PROC_SUSPENDED, 84   // offsetof(struct process, suspended)

.global handle_irq
handle_irq:
    sub   lr, lr, #4              // lr_irq = faulting PC

    // Determine if we interrupted user mode.
    // Use r0 as scratch (save/restore on IRQ stack; pop doesn't touch CPSR flags).
    push  {r0}
    mrs   r0, spsr
    and   r0, r0, #0x1F
    cmp   r0, #0x10               // usr = 0x10
    pop   {r0}                    // restore r0 (CPSR flags from cmp preserved)
    bne   .Lirq_not_user

    // ── User-mode preemption ──────────────────────────────────────────────────
    // r13_irq (banked SP_irq) used as scratch pointer — we switch to SVC for C calls.

    ldr   r13, =current_proc
    ldr   r13, [r13]              // r13_irq = process ptr
    add   r13, r13, #PROC_CTX     // r13_irq = &ctx

    stm   r13, {r0-r12}           // ctx.r[0..12] = user r0-r12

    // r0-r12 are now saved; use r0 as a stable (non-banked) copy of the ctx ptr.
    // We can't use r13_irq after a mode switch — it becomes sp_usr in System mode.
    mov   r0, r13                 // r0 = &ctx (non-banked, survives mode switch)

    // Save user SP/LR via System mode (shares user register banks)
    cps   #0x1F
    str   sp, [r0, #CTX_SP]       // ctx.sp = sp_usr  (r0 = &ctx, sp = sp_sys = sp_usr)
    str   lr, [r0, #CTX_LR]       // ctx.lr = lr_usr  (lr = lr_sys = lr_usr)
    cps   #0x12                   // back to IRQ mode  (r13_irq = &ctx again)

    str   lr, [r13, #CTX_PC]      // ctx.pc = lr_irq (user PC, already adjusted)
    mrs   r0, spsr
    str   r0, [r13, #CTX_CPSR]    // ctx.cpsr = spsr_irq (user CPSR)

    // Call irq_dispatch from SVC mode (uses the SVC stack)
    cps   #0x13
    bl    irq_dispatch

    // ── Check suspended flag ─────────────────────────────────────────────────
    ldr   r0, =current_proc
    ldr   r0, [r0]
    ldr   r1, [r0, #PROC_SUSPENDED]
    cmp   r1, #0
    bne   .Lirq_suspend

    // ── Restore user context ─────────────────────────────────────────────────
.Lirq_restore_user:
    // Entry: SVC mode, IRQs disabled.
    ldr   r1, =current_proc
    ldr   r1, [r1]
    add   r1, r1, #PROC_CTX       // r1 = &ctx

    ldr   r2, [r1, #CTX_CPSR]
    msr   spsr_cxsf, r2           // spsr_svc = user CPSR (for movs return)
    ldr   lr, [r1, #CTX_PC]       // lr_svc = user PC

    cps   #0x1F                   // System mode: restore user sp/lr
    ldr   sp, [r1, #CTX_SP]
    ldr   lr, [r1, #CTX_LR]
    cps   #0x13                   // SVC mode (lr_svc = user PC unchanged)

    ldm   r1, {r0-r12}            // r0-r12 restored (r1 base used, then overwritten)
    movs  pc, lr                  // CPSR ← spsr_svc, PC ← user PC

    // ── Suspend: idle in SVC mode until resumed ───────────────────────────────
.Lirq_suspend:
    cpsie i                       // enable IRQs so timer ticks can call irq_dispatch
.Lirq_check:
    ldr   r0, =current_proc
    ldr   r0, [r0]
    ldr   r1, [r0, #PROC_SUSPENDED]
    cmp   r1, #0
    beq   .Lirq_wfi_done
    wfi                           // sleep until next interrupt
    b     .Lirq_check
.Lirq_wfi_done:
    cpsid i                       // disable IRQs before restoring user context
    b     .Lirq_restore_user

    // ── Kernel-mode IRQ: fast path ────────────────────────────────────────────
.Lirq_not_user:
    push  {r0-r3, r12, lr}        // save volatile regs + lr_irq on IRQ stack
    bl    irq_dispatch
    pop   {r0-r3, r12, lr}
    movs  pc, lr                  // CPSR ← spsr_irq, PC ← kernel PC

// ── SVC trampoline ─────────────────────────────────────────────────────────────
//
// Stack layout after push {r1-r12, lr}  (13 regs × 4 = 52 bytes):
//   sp+0  = r1  ...  sp+44 = r12  sp+48 = lr_svc
// Then push {r12} (svc_num, 4 bytes) → sp 8-byte aligned before bl.
//
// C signature:  u32 svc_dispatch(u32 r0, u32 r1, u32 r2, u32 r3, u32 svc_num);
.global handle_svc
handle_svc:
    push {r1-r12, lr}           // save user r1-r12 and lr_svc; r0 stays as arg/retval
    ldr  r12, [lr, #-4]         // r12 = SVC instruction word (lr_svc still valid)
    and  r12, r12, #0xFFFFFF    // r12 = SVC number
    push {r12}                  // 5th arg to svc_dispatch (stack, per AAPCS)
    bl   svc_dispatch           // svc_dispatch(r0, r1, r2, r3, svc_num) → r0
    add  sp,  sp,  #4           // pop svc_num
    pop  {r1-r12, lr}           // restore user r1-r12 and lr_svc; r0 = return value
    movs pc,  lr                // return to user, restore CPSR from SPSR_svc
