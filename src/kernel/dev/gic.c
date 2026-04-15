#include "kernel/dev/gic.h"
#include "kernel/cpu.h"

void gic_enable_irq(u32 irq) {
    u32 reg             = irq / 32u;
    u32 bit             = 1u << (irq % 32u);
    GICD_ISENABLER(reg) = bit;

    // route to CPU0
    u32 idx   = irq / 4u;
    u32 shift = (irq % 4u) * 8u;
    u32 cur   = GICD_ITARGETSR(idx);
    cur &= ~(0xFFu << shift);
    cur |= (0x01u << shift);
    GICD_ITARGETSR(idx) = cur;

    GICD_IPRIORITYR(irq) = 0xA0u;
}

static u32 us_to_ticks(u32 usec) {
    // avoid 64-bit division: (freq / 1000) * (usec / 1000), ms-precision
    u32 freq_khz = read_cntfrq() / 1000u;
    u32 ms       = usec / 1000u;
    return freq_khz * (ms ? ms : 1u);
}

void timer_set_oneshot_us(u32 usec) {
    write_cntp_cval(read_cntpct() + us_to_ticks(usec));
    write_cntp_ctl((struct cntp_ctl) {.ENABLE = true});
}

// advance CVAL without touching CTL — no jitter from disable/re-enable
void timer_advance_cval(u32 usec) {
    write_cntp_cval(read_cntp_cval() + us_to_ticks(usec));
}

void timer_disable(void) {
    // IMASK suppresses interrupt output while ISTATUS stays set
    write_cntp_ctl((struct cntp_ctl) {.IMASK = true});
}
