//
// Created by Romir Kulshrestha on 09/05/2026.
//

#pragma once

#include "kernel/cpu.h"

void install_vtable();

void el1_sync_handler(cpu_ctx_t *ctx);
void el0_sync_handler(cpu_ctx_t *ctx);
