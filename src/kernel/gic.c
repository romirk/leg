#include "kernel/gic.h"

void gic_enable_irq(u32 irq) {
    u32 reg = irq / 32u;
    u32 bit = 1u << (irq % 32u);
    GICD_ISENABLER(reg) = bit;

    // route to CPU0
    u32 idx = irq / 4u;
    u32 shift = (irq % 4u) * 8u;
    u32 cur = GICD_ITARGETSR(idx);
    cur &= ~(0xFFu << shift);
    cur |= (0x01u << shift);
    GICD_ITARGETSR(idx) = cur;

    GICD_IPRIORITYR(irq) = 0xA0u;
}

static u32 read_cntfrq(void) {
    u32 val;
    asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(val));
    return val;
}

// CNTPCT — free-running counter (64-bit)
static u64 read_cntpct(void) {
    u32 lo, hi;
    asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(lo), "=r"(hi));
    return (u64) hi << 32 | lo;
}

// CNTP_CVAL — absolute compare value (64-bit); fires when CNTPCT >= CVAL
static u64 read_cntp_cval(void) {
    u32 lo, hi;
    asm volatile("mrrc p15, 2, %0, %1, c14" : "=r"(lo), "=r"(hi));
    return (u64) hi << 32 | lo;
}

static void write_cntp_cval(u64 val) {
    asm volatile("mcrr p15, 2, %Q0, %R0, c14" ::"r"(val));
}

static void write_cntp_ctl(u32 val) {
    asm volatile("mcr p15, 0, %0, c14, c2, 1" ::"r"(val));
}

static u32 us_to_ticks(u32 usec) {
    // avoid 64-bit division: (freq / 1000) * (usec / 1000), ms-precision
    u32 freq_khz = read_cntfrq() / 1000u;
    u32 ms = usec / 1000u;
    return freq_khz * (ms ? ms : 1u);
}

void timer_set_oneshot_us(u32 usec) {
    write_cntp_cval(read_cntpct() + us_to_ticks(usec));
    write_cntp_ctl(1u);
}

// advance CVAL without touching CTL — no jitter from disable/re-enable
void timer_advance_cval(u32 usec) {
    write_cntp_cval(read_cntp_cval() + us_to_ticks(usec));
}

void timer_disable(void) {
    // IMASK (bit 1) suppresses interrupt output while ISTATUS stays set
    write_cntp_ctl(0x2u);
}
