//
// Created by Romir Kulshrestha on 09/05/2026.
//

#include "types.h"
#include "utils.h"
#include "kernel/dev/gic.h"
#include "kernel/dev/uart.h"

[[noreturn]]
void kmain() {
    constexpr char msg[] = "kmain reached\n";
    for (const char *c = msg; *c; c++) {
        *UARTDR = *c;
    }

    gic_dist_init();
    gic_cpu_init();
    gic_enable_irq(TIMER_IRQ);

    asm volatile("msr daifclr, #2");

    // arm timer
    u64 now;
    asm volatile("mrs %0, cntpct_el0" : "=r"(now));
    asm volatile("msr cntp_cval_el0, %0" ::"r"(now + 50000000ULL)); // ~250ms at 200MHz
    asm volatile("msr cntp_ctl_el0, %0" ::"r"(1ULL));

    limbo;
}
