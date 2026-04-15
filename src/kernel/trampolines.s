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
// struct process offsets (must match process.h):
//   +0   pid
//   +4   pgd
//   +8   sp (initial)
//   +12  entry
//   +16  ctx.r[0..12]  (52 bytes)
//   +68  ctx.sp
//   +72  ctx.lr
//   +76  ctx.pc
//   +80  ctx.cpsr
//   +84  suspended
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
    push  {r0}
    mrs   r0, spsr
    and   r0, r0, #0x1F
    cmp   r0, #0x10               // usr = 0x10
    pop   {r0}
    bne   .Lirq_not_user

    // ── User-mode preemption ──────────────────────────────────────────────────
    // Load current_proc pointer directly — no bl, so lr_irq is still valid.
    ldr   r13, =current_proc
    ldr   r13, [r13]              // r13_irq = current_proc ptr
    cmp   r13, #0
    beq   .Lirq_not_user          // no current process

    add   r13, r13, #PROC_CTX     // r13_irq = &ctx

    stm   r13, {r0-r12}           // ctx.r[0..12] = user r0-r12

    mov   r0, r13                 // r0 = &ctx (non-banked, survives mode switch)

    // Save user SP/LR via System mode
    cps   #0x1F
    str   sp, [r0, #CTX_SP]
    str   lr, [r0, #CTX_LR]
    cps   #0x12                   // back to IRQ mode

    str   lr, [r13, #CTX_PC]      // ctx.pc = lr_irq (already adjusted)
    mrs   r0, spsr
    str   r0, [r13, #CTX_CPSR]

    // Call irq_dispatch from SVC mode
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
// Then push {r0} (4 bytes) + save lr_svc to svc_saved_lr + pop {r0}:
//   no net stack change for the lr save
// Then push {r12} (svc_num, 4 bytes) → sp 8-byte aligned before bl.
//
// svc_saved_lr is a global u32 written here so fork can read lr_svc from C.
//
// C signature:  u32 svc_dispatch(u32 r0, u32 r1, u32 r2, u32 r3, u32 svc_num);
.global handle_svc
handle_svc:
    push {r1-r12, lr}              // lr = lr_svc (return address in user code)
    ldr  r12, [lr, #-4]           // r12 = SVC instruction word; lr still = lr_svc
    and  r12, r12, #0xFFFFFF      // r12 = SVC number
    push {r0}                      // save r0 (SVC first argument)
    ldr  r0, =svc_saved_lr
    str  lr, [r0]                  // svc_saved_lr = lr_svc
    pop  {r0}                      // restore r0
    push {r12}                     // 5th arg = svc_num; stack is 8-byte aligned here
    bl   svc_dispatch
    add  sp,  sp,  #4

    // ── Blocking-syscall yield path ──────────────────────────────────────────
    // If the syscall set current_proc->suspended, save the full user context
    // from the SVC stack and hand off to the next runnable process.
    // Stack right now: sp+0=r1 … sp+44=r12 sp+48=lr_svc (13 words = 52 bytes)
    push  {r0, r1}
    ldr   r0, =current_proc
    ldr   r0, [r0]
    ldr   r1, [r0, #PROC_SUSPENDED]
    cmp   r1, #0
    pop   {r0, r1}
    bne   .Lsvc_yield

    // ── Normal return ─────────────────────────────────────────────────────────
    pop   {r1-r12, lr}
    movs  pc, lr

    // ── Yield: save context then switch ──────────────────────────────────────
    // r0 = SVC return value; stack: sp+0=saved_r1 … sp+44=saved_r12 sp+48=lr_svc
.Lsvc_yield:
    // Get ctx pointer into r2, keeping r0 intact via push/pop
    push  {r0}
    ldr   r0, =current_proc
    ldr   r0, [r0]
    add   r2, r0, #PROC_CTX        // r2 = &ctx
    pop   {r0}

    str   r0, [r2]                  // ctx.r[0] = return value

    // Save user sp/lr via System mode (r2 is non-banked, survives mode switch)
    cps   #0x1F
    str   sp, [r2, #CTX_SP]
    str   lr, [r2, #CTX_LR]
    cps   #0x13

    // Save user CPSR (= spsr_svc)
    mrs   r0, spsr
    str   r0, [r2, #CTX_CPSR]

    // Bulk-load saved r1-r12 from stack (skipping r2=ctx ptr) then stm into ctx.r[1..12].
    // ldm: r0←sp+0(r1), r3←sp+4(r2), r4←sp+8(r3), …, lr←sp+44(r12)
    // stm from ctx+4: r0→ctx.r[1], r3→ctx.r[2], …, lr→ctx.r[12]
    ldm   sp, {r0, r3-r12, lr}
    add   r2, r2, #4                // r2 = &ctx.r[1]
    stm   r2, {r0, r3-r12, lr}     // ctx.r[1..12]
    ldr   r0, [sp, #48]
    str   r0, [r2, #56]             // ctx.pc = lr_svc  (r2=ctx+4, +56 → ctx+60=CTX_PC)
    add   sp, sp, #52               // clean SVC frame

    // Pick the next runnable process and switch to it
    bl    sched_pick_next           // r0 = next or nullptr
    cmp   r0, #0
    beq   .Lsvc_no_next
    bl    context_switch            // jumps to next; never returns here

    // No other runnable process: spin in SVC mode until woken by sched_tick
.Lsvc_no_next:
    cpsie i
.Lsvc_spin:
    wfi
    ldr   r0, =current_proc
    ldr   r0, [r0]
    ldr   r0, [r0, #PROC_SUSPENDED]
    cmp   r0, #0
    bne   .Lsvc_spin
    cpsid i
    ldr   r0, =current_proc
    ldr   r0, [r0]
    bl    context_switch            // restore our own (or newly-current) context