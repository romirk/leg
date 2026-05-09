#pragma once

#include "types.h"

void handle_boot_exception(void);
void handle_prefetch_abort(void);
void handle_data_abort(void);
void handle_fiq(void);

// Called from the handle_irq assembly trampoline (boot.s).
void irq_dispatch(void);

void enable_interrupts(void);
void disable_interrupts(void);
