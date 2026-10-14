//
// Created by Romir Kulshrestha on 09/05/2026.
//

#pragma once

#include "types.h"
#include "kernel/arch/aarch64/boot.h"

typedef struct cpu_ctx {
    u64 x[31];
    u64 sp_el0;
    u64 elr_el1;
    u64 spsr_el1;
} cpu_ctx_t;
static_assert(sizeof(cpu_ctx_t) == FRAME_SIZE);
