//
// Created by Romir Kulshrestha on 09/05/2026.
//

#include "kernel/exceptions.h"

#include "syscall.h"
#include "types.h"
#include "utils.h"
#include "kernel/dev/gic.h"
#include "kernel/dev/uart.h"

static void uart_hex64(u64 val) {
    for (int i = 60; i >= 0; i -= 4)
        *UARTDR = "0123456789ABCDEF"[(val >> i) & 0xF];
}

// DEBUG: remove once exceptions are stable
static void dump_el1_sync(const u64 esr, const u64 far, const u64 elr) {
    const u8 ec = (esr >> 26) & 0x3F;
    *UARTDR     = 'E';
    *UARTDR     = '1';
    *UARTDR     = ' ';
    *UARTDR     = "0123456789ABCDEF"[ec >> 4];
    *UARTDR     = "0123456789ABCDEF"[ec & 0xF];
    *UARTDR     = ' ';
    uart_hex64(far);
    *UARTDR = ' ';
    uart_hex64(elr);
    *UARTDR = '\n';
}

void el1_sync_handler(cpu_ctx_t *) {
    u64 esr, far, elr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    asm volatile("mrs %0, far_el1" : "=r"(far));
    asm volatile("mrs %0, elr_el1" : "=r"(elr));
    dump_el1_sync(esr, far, elr);
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

void el0_irq_handler(cpu_ctx_t *) {
    const u32 irq_id = GICC_IAR;
    *UARTDR          = 'I';
    *UARTDR          = irq_id;
    *UARTDR          = '\n';
    GICC_EOIR        = irq_id;
}

uptr svc_dispatch(uptr, uptr, uptr, uptr, const uptr svc_num) {
    // TODO use the one in syscall.c
    *UARTDR = 'S';
    *UARTDR = "0123456789ABCDEF"[svc_num >> 4];
    *UARTDR = "0123456789ABCDEF"[svc_num & 0xF];
    *UARTDR = '\n';
    return svc_num;
}
