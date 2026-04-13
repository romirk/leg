// syscall.h — userspace system call interface
//
// Each SVC transfers control to the kernel via the ARM software interrupt mechanism.
// Arguments follow the ARM EABI register convention (r0–r3); the return value is in r0.
// SVCs are safe to call from any context where interrupts are otherwise valid.

#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

// ─── SVC numbers ─────────────────────────────────────────────────────────────

// Process
#define SVC_EXIT 0

// I/O
#define SVC_WRITE      1
#define SVC_READ       2
#define SVC_GETCHAR    3
#define SVC_GETCHAR_NB 4

// Timing
#define SVC_SLEEP  5
#define SVC_TICKS  6
#define SVC_CNTFRQ 7

// Framebuffer
#define SVC_FB_CLEAR    8
#define SVC_FB_PUTPIXEL 9
#define SVC_FB_PUTC     10 // col_row = (col << 16) | row

// RNG
#define SVC_RAND      11
#define SVC_RAND_SEED 12

// Debug
#define SVC_UART_WRITE 13 // write buf directly to UART (bypasses framebuffer)

// Block device
#define SVC_BLK_READ  14 // r0=sector_lo, r1=sector_hi, r2=count, r3=buf → bool
#define SVC_BLK_WRITE 15 // r0=sector_lo, r1=sector_hi, r2=count, r3=buf → bool
// ─── Process ─────────────────────────────────────────────────────────────────

[[noreturn, maybe_unused]]
static inline void sys_exit(int code) {
    register int r0 asm("r0") = code;
    // No return, but we list clobbers for correctness during the call itself
    asm volatile("svc #0" ::"r"(r0) : "r1", "r2", "r3", "ip", "lr", "cc", "memory");
    __builtin_unreachable();
}

// ─── I/O ─────────────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline u32 sys_write(const char *buf, u32 len) {
    register u32 r0 asm("r0") = (u32) buf;
    register u32 r1 asm("r1") = len;
    asm volatile("svc #1" : "+r"(r0) : "r"(r1) : "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

[[maybe_unused]]
static inline u32 sys_read(char *buf, u32 max) {
    register u32 r0 asm("r0") = (u32) buf;
    register u32 r1 asm("r1") = max;
    asm volatile("svc #2" : "+r"(r0) : "r"(r1) : "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

[[maybe_unused]]
static inline char sys_getchar(void) {
    register u32 r0 asm("r0");
    asm volatile("svc #3" : "=r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
    return (char) r0;
}

[[maybe_unused]]
static inline char sys_getchar_nb(void) {
    register u32 r0 asm("r0");
    asm volatile("svc #4" : "=r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
    return (char) r0;
}

// ─── Timing ──────────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline void sys_sleep(u32 us) {
    register u32 r0 asm("r0") = us;
    asm volatile("svc #5" : "+r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
}

[[maybe_unused]]
static inline void sys_ticks(u64 *out) {
    register u32 r0 asm("r0") = (u32) out;
    asm volatile("svc #6" : "+r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
}

[[maybe_unused]]
static inline u32 sys_cntfrq(void) {
    register u32 r0 asm("r0");
    asm volatile("svc #7" : "=r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

// ─── Framebuffer ─────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline void sys_fb_clear(u32 color) {
    register u32 r0 asm("r0") = color;
    asm volatile("svc #8" : "+r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
}

[[maybe_unused]]
static inline void sys_fb_putpixel(u32 x, u32 y, u32 color) {
    register u32 r0 asm("r0") = x;
    register u32 r1 asm("r1") = y;
    register u32 r2 asm("r2") = color;
    // Only r3 is left as a scratch register to clobber
    asm volatile("svc #9" : "+r"(r0) : "r"(r1), "r"(r2) : "r3", "ip", "lr", "cc", "memory");
}

[[maybe_unused]]
static inline void sys_fb_putc(u32 col_row, char c, u32 fg, u32 bg) {
    register u32 r0 asm("r0") = col_row;
    register u32 r1 asm("r1") = (u32) c;
    register u32 r2 asm("r2") = fg;
    register u32 r3 asm("r3") = bg;
    // All arg registers r0-r3 are used, so none are in the clobber list
    asm volatile("svc #10" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3) : "ip", "lr", "cc", "memory");
}

// ─── RNG ─────────────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline u32 sys_rand(void) {
    register u32 r0 asm("r0");
    asm volatile("svc #11" : "=r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

[[maybe_unused]]
static inline void sys_rand_seed(u32 seed) {
    register u32 r0 asm("r0") = seed;
    asm volatile("svc #12" : "+r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
}

// ─── Debug ────────────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline u32 sys_uart_write(const char *buf, u32 len) {
    register u32 r0 asm("r0") = (u32) buf;
    register u32 r1 asm("r1") = len;
    asm volatile("svc #13" : "+r"(r0) : "r"(r1) : "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

// ─── Block device ─────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline bool sys_blk_read(u64 sector, u32 count, void *buf) {
    register u32 r0 asm("r0") = (u32) sector;
    register u32 r1 asm("r1") = (u32) (sector >> 32);
    register u32 r2 asm("r2") = count;
    register u32 r3 asm("r3") = (u32) buf;
    asm volatile("svc #14" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3) : "ip", "lr", "cc", "memory");
    return (bool) r0;
}

[[maybe_unused]]
static inline bool sys_blk_write(u64 sector, u32 count, const void *buf) {
    register u32 r0 asm("r0") = (u32) sector;
    register u32 r1 asm("r1") = (u32) (sector >> 32);
    register u32 r2 asm("r2") = count;
    register u32 r3 asm("r3") = (u32) buf;
    asm volatile("svc #15" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3) : "ip", "lr", "cc", "memory");
    return (bool) r0;
}

#endif // SYSCALL_H