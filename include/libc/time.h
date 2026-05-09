// time.h — userspace timing API

#pragma once

#include "syscall.h"
#include "types.h"

// Return the current ARM generic timer counter value (CNTPCT).
static inline u64 get_ticks(void) {
    u64 t;
    sys_ticks(&t);
    return t;
}

// Return the timer counter frequency in Hz (CNTFRQ).
static inline u32 cntfrq(void) {
    return sys_cntfrq();
}
