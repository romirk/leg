//
// Created by Romir Kulshrestha on 17/08/2025.
//

#include "kernel/rtc.h"
#include "kernel/gic.h"

static volatile bool timer_pending;

static void (*tick_fn)(void);

static u32 tick_interval_us;

u64 rtc_ticks(void) {
    u32 lo, hi;
    asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(lo), "=r"(hi));
    return (u64) hi << 32 | lo;
}

void timer_set_tick(u32 interval_us, void (*fn)(void)) {
    tick_interval_us = interval_us;
    tick_fn = fn;
    timer_set_oneshot_us(interval_us);
}

void timer_clear_tick(void) {
    tick_fn = nullptr;
    tick_interval_us = 0;
    timer_disable();
}

void rtc_timer_fired(void) {
    if (tick_fn) {
        // advance CVAL by the interval — timer stays enabled, no jitter from re-arm
        timer_advance_cval(tick_interval_us);
        tick_fn();
    } else {
        timer_disable();
    }

    timer_pending = false;
}

void delay_us(u32 usec) {
    // incompatible with timer_set_tick — once a scheduler tick is running,
    // use a scheduler sleep instead
    timer_pending = true;
    timer_set_oneshot_us(usec);
    while (timer_pending)
        asm volatile("wfi");
}
