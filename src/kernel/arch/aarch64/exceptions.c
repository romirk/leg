//
// Created by Romir Kulshrestha on 09/05/2026.
//

#include "kernel/exceptions.h"

#include "syscall.h"
#include "types.h"
#include "utils.h"

#define UARTDR (volatile u8 *) 0x09000000

extern byte vectors[];

void install_vtable() {
    asm volatile("msr vbar_el1, %0" ::"r"(vectors));
}

void el1_sync_handler(cpu_ctx_t *ctx) {
    *UARTDR = 'E';
    *UARTDR = '1';
    *UARTDR = '\n';
    limbo;
}

void el0_sync_handler(cpu_ctx_t *ctx) {
    u64 esr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));

    const u8 ec = (esr >> 26) & 0b111111;
    if (ec != 0x15) return;

    const u16 iss = esr & 0xFFFF;
    ctx->x[0]     = svc_dispatch(ctx->x[0], ctx->x[1], ctx->x[2], ctx->x[3], iss);
}

uptr svc_dispatch(uptr, uptr, uptr, uptr, const uptr svc_num) {
    // TODO use the one in syscall.c
    *UARTDR = svc_num;
    *UARTDR = '\n';
    return svc_num;
}
