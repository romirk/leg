#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "types.h"

void handle_boot_exception(void);
void handle_data_abort(void);
void handle_fiq(void);

// Called from the handle_irq assembly trampoline (boot.s).
void irq_dispatch(void);

// Called from the handle_svc assembly trampoline (boot.s).
u32 svc_dispatch(u32 r0, u32 r1, u32 r2, u32 r3, u32 svc_num);

void enable_interrupts(void);
void disable_interrupts(void);

#endif // EXCEPTIONS_H