#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

#define SVC_EXIT  0
#define SVC_WRITE 1
#define SVC_READ  2
#define SVC_SLEEP 3

[[noreturn, maybe_unused]]
static void sys_exit(int code) {
    register int r0 asm("r0") = code;
    asm volatile("svc #0" ::"r"(r0));
    __builtin_unreachable();
}

// Write len bytes from buf to the TTY. Returns bytes written.
[[maybe_unused]]
static u32 sys_write(const char *buf, u32 len) {
    register u32 r0 asm("r0") = (u32) buf;
    register u32 r1 asm("r1") = len;
    asm volatile("svc #1" : "+r"(r0) : "r"(r1));
    return r0;
}

// Read one line from the TTY into buf (max bytes). Blocks until \n. Returns byte count.
[[maybe_unused]]
static u32 sys_read(char *buf, u32 max) {
    register u32 r0 asm("r0") = (u32) buf;
    register u32 r1 asm("r1") = max;
    asm volatile("svc #2" : "+r"(r0) : "r"(r1));
    return r0;
}

// Sleep for us microseconds.
[[maybe_unused]]
static void sys_sleep(u32 us) {
    register u32 r0 asm("r0") = us;
    asm volatile("svc #3" ::"r"(r0));
}

#endif // SYSCALL_H