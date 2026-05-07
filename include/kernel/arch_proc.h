//
// Created by Romir Kulshrestha on 07/05/2026.
//

#ifndef LEG_ARCH_PROC_H
#define LEG_ARCH_PROC_H
#include "types.h"

typedef struct cpu_ctx cpu_ctx_t;

void arch_ctx_init_fresh(cpu_ctx_t *ctx, uptr entry, uptr sp);
void arch_ctx_init_fork(cpu_ctx_t *child, const cpu_ctx_t *parent, uptr lr_svc, uptr sp_usr,
                        uptr arch_state);
[[noreturn]]
void arch_eret_to_user(const cpu_ctx_t *ctx);
void arch_get_user_fork_state(uptr *out_sp, uptr *out_state);
void arch_ctx_set_syscall_return(cpu_ctx_t *ctx, uptr value);

#endif // LEG_ARCH_PROC_H
