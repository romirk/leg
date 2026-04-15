#include "kernel/exceptions.h"

#include "kernel/cpu.h"
#include "kernel/dev/gic.h"
#include "kernel/dev/kbd.h"
#include "kernel/dev/rtc.h"
#include "kernel/dev/uart.h"
#include "types.h"
#include "utils.h"

// Interrupt handlers only call integer-only kernel paths; VFP state is not touched.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warm-interrupt-vfp-clobber"

[[gnu::section(".startup.exceptions")]]
void handle_boot_exception(void) {
    *UARTDR = '!';
    *UARTDR = '\n';

    poweroff();
}

[[gnu::interrupt("ABORT")]]
void handle_data_abort(void) {
    u32 dfar, dfsr, pc;
    asm volatile("mrc p15, 0, %0, c6, c0, 0" : "=r"(dfar));
    asm volatile("mrc p15, 0, %0, c5, c0, 0" : "=r"(dfsr));
    asm volatile("mov %0, lr" : "=r"(pc)); // lr_abt = faulting PC + 8
    panic("data abort: fa=%p fs=%p pc=%p", (void *) dfar, (void *) dfsr, (void *) (pc - 8));
}

// Called from the handle_irq assembly trampoline in boot.s.
// Normal C function (no interrupt attribute) — the trampoline manages context.
void irq_dispatch(void) {
    const u32 id = GICC_IAR & 0x3FFu;

    if (id == TIMER_IRQ) {
        rtc_timer_fired();
    } else if (id == UART_IRQ) {
        uart_irq_handler();
    } else if (kbd_irq && id == kbd_irq) {
        kbd_irq_handler();
    }

    GICC_EOIR = id;
}

[[gnu::interrupt("FIQ")]]
void handle_fiq(void) {
    *UARTDR = '?';
}

#pragma clang diagnostic pop

void enable_interrupts(void) {
    auto cpsr = read_cpsr();
    cpsr.I    = false;
    cpsr.A    = false;
    cpsr.F    = false;
    write_cpsr(cpsr);
}

void disable_interrupts(void) {
    auto cpsr = read_cpsr();
    cpsr.I    = true;
    cpsr.A    = true;
    cpsr.F    = true;
    write_cpsr(cpsr);
}
