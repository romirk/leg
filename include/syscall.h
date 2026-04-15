// syscall.h — userspace system call interface
//
// Each SVC transfers control to the kernel via the ARM software interrupt mechanism.
// Arguments follow the ARM EABI register convention (r0–r3); the return value is in r0.
// SVCs are safe to call from any context where interrupts are otherwise valid.

#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

#ifndef STR_HELPER
#define STR_HELPER(X) #X
#define STR(X)        STR_HELPER(X)
#endif

// ─── SVC numbers ─────────────────────────────────────────────────────────────

// Process
#define SVC_EXIT   0
#define SVC_FORK   1 // → pid_t: child gets 0, parent gets child PID
#define SVC_GETPID 2 // → pid_t: current process PID
#define SVC_JOIN   3 // r0=pid → exit code, or -1 if pid not found

// I/O
#define SVC_WRITE      4
#define SVC_READ       5
#define SVC_GETCHAR    6
#define SVC_GETCHAR_NB 7

// Timing
#define SVC_SLEEP  7
#define SVC_TICKS  8
#define SVC_CNTFRQ 9

// Framebuffer
#define SVC_FB_CLEAR    10
#define SVC_FB_PUTPIXEL 11
#define SVC_FB_PUTC     12 // col_row = (col << 16) | row

// RNG
#define SVC_RAND      13
#define SVC_RAND_SEED 14

// Debug
#define SVC_UART_WRITE 15 // write buf directly to UART (bypasses framebuffer)

// Block device
#define SVC_BLK_READ  16 // r0=sector_lo, r1=sector_hi, r2=count, r3=buf → bool
#define SVC_BLK_WRITE 17 // r0=sector_lo, r1=sector_hi, r2=count, r3=buf → bool

// Filesystem
#define SVC_FS_BLOB_COUNT 18 // → u32 count
#define SVC_FS_BLOB_INFO  19 // r0=index, r1=name_buf, r2=buf_size, r3=*size_out → name_len

// ─── Process ─────────────────────────────────────────────────────────────────

[[noreturn, maybe_unused]]
static inline void sys_exit(int code) {
    register int r0 asm("r0") = code;
    asm volatile("svc #" STR(SVC_EXIT)::"r"(r0) : "r1", "r2", "r3", "ip", "lr", "cc", "memory");
    __builtin_unreachable();
}

// Returns child PID in parent, 0 in child.
[[maybe_unused]]
static inline u32 sys_fork(void) {
    register u32 r0 asm("r0");
    asm volatile("svc #" STR(SVC_FORK) : "=r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

[[maybe_unused]]
static inline u32 sys_getpid(void) {
    register u32 r0 asm("r0");
    asm volatile("svc #" STR(SVC_GETPID) : "=r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

// ─── I/O ─────────────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline u32 sys_write(const char *buf, u32 len) {
    register u32 r0 asm("r0") = (u32) buf;
    register u32 r1 asm("r1") = len;
    asm volatile("svc #" STR(SVC_WRITE)
                 : "+r"(r0)
                 : "r"(r1)
                 : "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

[[maybe_unused]]
static inline u32 sys_read(char *buf, u32 max) {
    register u32 r0 asm("r0") = (u32) buf;
    register u32 r1 asm("r1") = max;
    asm volatile("svc #" STR(SVC_READ)
                 : "+r"(r0)
                 : "r"(r1)
                 : "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

[[maybe_unused]]
static inline char sys_getchar(void) {
    register u32 r0 asm("r0");
    asm volatile("svc #" STR(SVC_GETCHAR) : "=r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
    return (char) r0;
}

[[maybe_unused]]
static inline char sys_getchar_nb(void) {
    register u32 r0 asm("r0");
    asm volatile("svc #" STR(SVC_GETCHAR_NB)
                 : "=r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
    return (char) r0;
}

// ─── Timing ──────────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline void sys_sleep(u32 us) {
    register u32 r0 asm("r0") = us;
    asm volatile("svc #" STR(SVC_SLEEP) : "+r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
}

[[maybe_unused]]
static inline void sys_ticks(u64 *out) {
    register u32 r0 asm("r0") = (u32) out;
    asm volatile("svc #" STR(SVC_TICKS) : "+r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
}

[[maybe_unused]]
static inline u32 sys_cntfrq(void) {
    register u32 r0 asm("r0");
    asm volatile("svc #" STR(SVC_CNTFRQ) : "=r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

// ─── Framebuffer ─────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline void sys_fb_clear(u32 color) {
    register u32 r0 asm("r0") = color;
    asm volatile("svc #" STR(SVC_FB_CLEAR)
                 : "+r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
}

[[maybe_unused]]
static inline void sys_fb_putpixel(u32 x, u32 y, u32 color) {
    register u32 r0 asm("r0") = x;
    register u32 r1 asm("r1") = y;
    register u32 r2 asm("r2") = color;
    asm volatile("svc #" STR(SVC_FB_PUTPIXEL)
                 : "+r"(r0)
                 : "r"(r1), "r"(r2)
                 : "r3", "ip", "lr", "cc", "memory");
}

[[maybe_unused]]
static inline void sys_fb_putc(u32 col_row, char c, u32 fg, u32 bg) {
    register u32 r0 asm("r0") = col_row;
    register u32 r1 asm("r1") = (u32) c;
    register u32 r2 asm("r2") = fg;
    register u32 r3 asm("r3") = bg;
    asm volatile("svc #" STR(SVC_FB_PUTC)
                 : "+r"(r0)
                 : "r"(r1), "r"(r2), "r"(r3)
                 : "ip", "lr", "cc", "memory");
}

// ─── RNG ─────────────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline u32 sys_rand(void) {
    register u32 r0 asm("r0");
    asm volatile("svc #" STR(SVC_RAND) : "=r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

[[maybe_unused]]
static inline void sys_rand_seed(u32 seed) {
    register u32 r0 asm("r0") = seed;
    asm volatile("svc #" STR(SVC_RAND_SEED)
                 : "+r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
}

// ─── Debug ────────────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline u32 sys_uart_write(const char *buf, u32 len) {
    register u32 r0 asm("r0") = (u32) buf;
    register u32 r1 asm("r1") = len;
    asm volatile("svc #" STR(SVC_UART_WRITE)
                 : "+r"(r0)
                 : "r"(r1)
                 : "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

// ─── Block device ─────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline bool sys_blk_read(u64 sector, u32 count, void *buf) {
    register u32 r0 asm("r0") = (u32) sector;
    register u32 r1 asm("r1") = (u32) (sector >> 32);
    register u32 r2 asm("r2") = count;
    register u32 r3 asm("r3") = (u32) buf;
    asm volatile("svc #" STR(SVC_BLK_READ)
                 : "+r"(r0)
                 : "r"(r1), "r"(r2), "r"(r3)
                 : "ip", "lr", "cc", "memory");
    return (bool) r0;
}

[[maybe_unused]]
static inline bool sys_blk_write(u64 sector, u32 count, const void *buf) {
    register u32 r0 asm("r0") = (u32) sector;
    register u32 r1 asm("r1") = (u32) (sector >> 32);
    register u32 r2 asm("r2") = count;
    register u32 r3 asm("r3") = (u32) buf;
    asm volatile("svc #" STR(SVC_BLK_WRITE)
                 : "+r"(r0)
                 : "r"(r1), "r"(r2), "r"(r3)
                 : "ip", "lr", "cc", "memory");
    return (bool) r0;
}

// ─── Filesystem ──────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline u32 sys_fs_blob_count(void) {
    register u32 r0 asm("r0");
    asm volatile("svc #" STR(SVC_FS_BLOB_COUNT)
                 : "=r"(r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");
    return r0;
}

// Returns name length, or 0 if index is out of range.
// name_buf receives a null-terminated name; size_out receives blob data size.
[[maybe_unused]]
static inline u32 sys_fs_blob_info(u32 index, char *name_buf, u32 name_buf_size, u32 *size_out) {
    register u32 r0 asm("r0") = index;
    register u32 r1 asm("r1") = (u32) name_buf;
    register u32 r2 asm("r2") = name_buf_size;
    register u32 r3 asm("r3") = (u32) size_out;
    asm volatile("svc #" STR(SVC_FS_BLOB_INFO)
                 : "+r"(r0)
                 : "r"(r1), "r"(r2), "r"(r3)
                 : "ip", "lr", "cc", "memory");
    return r0;
}

#endif // SYSCALL_H