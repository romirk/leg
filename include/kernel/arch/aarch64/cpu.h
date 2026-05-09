//
// Created by Romir Kulshrestha on 09/05/2026.
//

#pragma once

#define FRAME_SIZE 272 // usable from both C and .S

#ifndef __ASSEMBLER__

#include "types.h"
typedef struct cpu_ctx {
    u64 x[31];
    u64 sp_el0;
    u64 elr_el1;
    u64 spsr_el1;
} cpu_ctx_t;
static_assert(sizeof(cpu_ctx_t) == FRAME_SIZE);

#endif
