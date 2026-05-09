//
// Created by Romir Kulshrestha on 09/05/2026.
//

#include "kernel/exceptions.h"
#include "types.h"

#define UARTDR (volatile u8 *) 0x09000000

extern byte vectors[];

void install_vtable() {
    asm volatile("msr vbar_el1, %0" ::"r"(vectors));
}

void el1_sync_handler(cpu_ctx_t *ctx) {
    u64 esr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));

    // skip to next instruction
    ctx->elr_el1 += 4;

    *UARTDR = (esr >> 26) & 0b111111;
    *UARTDR = '\n';
}