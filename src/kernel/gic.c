//
// Created by Romir Kulshrestha on 05/06/2025.
//

#include "kernel/gic.h"

/* enable single IRQ (any irq number) */
void gic_enable_irq(u32 irq) {
    u32 reg = irq / 32u;
    u32 bit = 1u << (irq % 32u);
    GICD_ISENABLER(reg) = bit;

    /* route this IRQ to CPU0: set ITARGETSR byte corresponding to irq */
    u32 idx = irq / 4u;
    /* set each byte; keep it simple and target CPU0 (0x01) */
    u32 shift = (irq % 4u) * 8u;
    u32 cur = GICD_ITARGETSR(idx);
    cur &= ~(0xFFu << shift);
    cur |= (0x01u << shift);
    GICD_ITARGETSR(idx) = cur;

    /* optional: set priority (lower = higher priority) */
    GICD_IPRIORITYR(irq) = 0xA0u; /* mid priority */
}

/* --- ARM Generic Timer sysreg helpers (ARMv7 generic timer) --- */
static u32 read_cntfrq(void) {
    u32 val;
    asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(val));
    return val;
}

static void write_cntp_tval(u32 val) {
    asm volatile("mcr p15, 0, %0, c14, c2, 1" :: "r"(val));
}

static void write_cntp_ctl(u32 val) {
    asm volatile("mcr p15, 0, %0, c14, c2, 0" :: "r"(val));
}

/* --- Timer setup: set timeout in microseconds --- */
void timer_set_oneshot_us(u32 usec) {
    u32 freq = read_cntfrq(); /* ticks per second */
    u64 ticks = (u64) freq * usec / 1000000u;
    if (ticks == 0) ticks = 1;
    write_cntp_tval((u32) ticks);
    /* enable the physical non-secure timer (CNTPL?) - use CNTP_CTL: bit0 = enable */
    write_cntp_ctl(1u);
}

// /* --- Example usage in main --- */
// int main(void)
// {
//     /* initialize GIC */
//     gic_cpu_init();
//     gic_dist_init();
//
//     /* enable the timer IRQ found in device tree */
//     gic_enable_irq(TIMER_IRQ);
//
//     /* program timer for 1000000 us (1s) */
//     timer_set_oneshot_us(1000000u);
//
//     /* enable IRQs globally */
//     asm volatile("cpsie i");
//
//     /* main loop */
//     while (1) {
//         /* wait for interrupts (wfi) */
//         asm volatile("wfi");
//     }
//
//     return 0;
// }
