//
// Created by Romir Kulshrestha on 07/05/2026.
//

#include "kernel/arch_proc.h"

#include "kernel/arch/arm32/cpu.h"
#include "kernel/process.h"
#include "libc/builtins.h"

void arch_ctx_init_fresh(cpu_ctx_t *ctx, uptr entry, uptr sp) {
    memclr(ctx, sizeof(cpu_ctx_t));
    ctx->sp   = sp;
    ctx->pc   = entry;
    ctx->cpsr = 0x10u; // USR mode, IRQs enabled
}

void arch_ctx_init_fork(cpu_ctx_t *child, const cpu_ctx_t *parent, uptr lr_svc, uptr sp_usr,
                        uptr arch_state) {
    memcpy(child, parent, sizeof(cpu_ctx_t));
    child->sp   = sp_usr;
    child->pc   = lr_svc;
    child->cpsr = arch_state;
    arch_ctx_set_syscall_return(child, 0);
}

[[noreturn]]
void arch_eret_to_user(const cpu_ctx_t *ctx) {
    write_spsr(__builtin_bit_cast(psr_t, ctx->cpsr));
    asm volatile("cps #31          \n\t" // System Mode
                 "mov sp, %0       \n\t" // Set sp_usr
                 "cps #19          \n\t" // Back to SVC Mode
                 "mov lr, %1       \n\t" // entry point
                 "movs pc, lr      \n\t" // Mode switch
                 :
                 : "r"(ctx->sp), "r"(ctx->pc)
                 : "memory");
    __builtin_unreachable();
}

void arch_get_user_fork_state(uptr *out_sp, uptr *out_state) {
    asm volatile("cps #0x1F \n\t" // System mode — shares user register bank
                 "mov %0, sp \n\t"
                 "cps #0x13 \n\t" // back to SVC mode
                 "mrs %1, spsr"   // spsr_svc = user CPSR
                 : "=r"(*out_sp), "=r"(*out_state)
                 :
                 : "lr"); // lr is banked across cps; clobber so compiler picks r0-r12 for outputs
}

void arch_ctx_set_syscall_return(cpu_ctx_t *ctx, const uptr value) {
    ctx->r[0] = value;
}