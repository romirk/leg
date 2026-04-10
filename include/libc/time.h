// time.h — userspace timing API

#ifndef LEG_TIME_H
#define LEG_TIME_H

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

#endif // LEG_TIME_H
