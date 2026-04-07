//
// Created by Romir Kulshrestha on 17/08/2025.
//

#include "kernel/rtc.h"
#include "kernel/gic.h"

static volatile bool timer_pending;

u64 rtc_ticks(void) {
    u32 lo, hi;
    asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(lo), "=r"(hi));
    return (u64) hi << 32 | lo;
}

void rtc_timer_fired(void) {
    timer_disable();
    timer_pending = false;
}

void delay_us(u32 usec) {
    timer_pending = true;
    timer_set_oneshot_us(usec);
    while (timer_pending)
        asm volatile("wfi");
}
