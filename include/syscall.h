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

// ─── SVC dispatch helpers ─────────────────────────────────────────────────────

#define SVC0(N)                                                                                    \
    ({                                                                                             \
        register u32 _r0 asm("r0");                                                                \
        asm volatile("svc #" STR(N) : "=r"(_r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");    \
        _r0;                                                                                       \
    })

#define SVC1(N, a0)                                                                                \
    ({                                                                                             \
        register u32 _r0 asm("r0") = (u32) (a0);                                                   \
        asm volatile("svc #" STR(N) : "+r"(_r0)::"r1", "r2", "r3", "ip", "lr", "cc", "memory");    \
        _r0;                                                                                       \
    })

#define SVC2(N, a0, a1)                                                                            \
    ({                                                                                             \
        register u32 _r0 asm("r0") = (u32) (a0);                                                   \
        register u32 _r1 asm("r1") = (u32) (a1);                                                   \
        asm volatile("svc #" STR(N)                                                                \
                     : "+r"(_r0)                                                                   \
                     : "r"(_r1)                                                                    \
                     : "r2", "r3", "ip", "lr", "cc", "memory");                                    \
        _r0;                                                                                       \
    })

#define SVC3(N, a0, a1, a2)                                                                        \
    ({                                                                                             \
        register u32 _r0 asm("r0") = (u32) (a0);                                                   \
        register u32 _r1 asm("r1") = (u32) (a1);                                                   \
        register u32 _r2 asm("r2") = (u32) (a2);                                                   \
        asm volatile("svc #" STR(N)                                                                \
                     : "+r"(_r0)                                                                   \
                     : "r"(_r1), "r"(_r2)                                                          \
                     : "r3", "ip", "lr", "cc", "memory");                                          \
        _r0;                                                                                       \
    })

#define SVC4(N, a0, a1, a2, a3)                                                                    \
    ({                                                                                             \
        register u32 _r0 asm("r0") = (u32) (a0);                                                   \
        register u32 _r1 asm("r1") = (u32) (a1);                                                   \
        register u32 _r2 asm("r2") = (u32) (a2);                                                   \
        register u32 _r3 asm("r3") = (u32) (a3);                                                   \
        asm volatile("svc #" STR(N)                                                                \
                     : "+r"(_r0)                                                                   \
                     : "r"(_r1), "r"(_r2), "r"(_r3)                                                \
                     : "ip", "lr", "cc", "memory");                                                \
        _r0;                                                                                       \
    })

// ─── SVC numbers ─────────────────────────────────────────────────────────────

// Process
#define SVC_EXIT   0
#define SVC_FORK   1 // → pid_t: child gets 0, parent gets child PID
#define SVC_GETPID 2 // → pid_t: current process PID
#define SVC_JOIN   3 // r0=pid → exit code, or -1 if pid not found
#define SVC_EXEC   4

// I/O
#define SVC_WRITE      5
#define SVC_READ       6
#define SVC_GETCHAR    7
#define SVC_GETCHAR_NB 8

// Timing
#define SVC_SLEEP  9
#define SVC_TICKS  10
#define SVC_CNTFRQ 11

// Framebuffer
#define SVC_FB_CLEAR    12
#define SVC_FB_PUTPIXEL 13
#define SVC_FB_PUTC     14 // col_row = (col << 16) | row

// RNG
#define SVC_RAND      15
#define SVC_RAND_SEED 16

// Debug
#define SVC_UART_WRITE 17 // write buf directly to UART (bypasses framebuffer)

// Block device
#define SVC_BLK_READ  18 // r0=sector_lo, r1=sector_hi, r2=count, r3=buf → bool
#define SVC_BLK_WRITE 19 // r0=sector_lo, r1=sector_hi, r2=count, r3=buf → bool

// Filesystem
#define SVC_FS_BLOB_COUNT 20 // → u32 count
#define SVC_FS_BLOB_INFO  21 // r0=index, r1=name_buf, r2=buf_size, r3=*size_out → name_len

// ─── Process ─────────────────────────────────────────────────────────────────

[[noreturn, maybe_unused]]
static inline void sys_exit(int code) {
    SVC1(SVC_EXIT, code);
    __builtin_unreachable();
}

// Returns child PID in parent, 0 in child.
[[maybe_unused]]
static inline u32 sys_fork(void) {
    return SVC0(SVC_FORK);
}

[[maybe_unused]]
static inline u32 sys_getpid(void) {
    return SVC0(SVC_GETPID);
}

[[maybe_unused]]
static inline int sys_join(u32 pid) {
    return (int) SVC1(SVC_JOIN, pid);
}

[[maybe_unused]]
static inline u32 sys_exec(const char *name) {
    return SVC1(SVC_EXEC, name);
}

// ─── I/O ─────────────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline u32 sys_write(const char *buf, u32 len) {
    return SVC2(SVC_WRITE, buf, len);
}
[[maybe_unused]]
static inline u32 sys_read(char *buf, u32 max) {
    return SVC2(SVC_READ, buf, max);
}
[[maybe_unused]]
static inline char sys_getchar(void) {
    return (char) SVC0(SVC_GETCHAR);
}
[[maybe_unused]]
static inline char sys_getchar_nb(void) {
    return (char) SVC0(SVC_GETCHAR_NB);
}

// ─── Timing ──────────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline void sys_sleep(u32 us) {
    SVC1(SVC_SLEEP, us);
}
[[maybe_unused]]
static inline void sys_ticks(u64 *out) {
    SVC1(SVC_TICKS, out);
}
[[maybe_unused]]
static inline u32 sys_cntfrq(void) {
    return SVC0(SVC_CNTFRQ);
}

// ─── Framebuffer ─────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline void sys_fb_clear(u32 color) {
    SVC1(SVC_FB_CLEAR, color);
}
[[maybe_unused]]
static inline void sys_fb_putpixel(u32 x, u32 y, u32 color) {
    SVC3(SVC_FB_PUTPIXEL, x, y, color);
}
[[maybe_unused]]
static inline void sys_fb_putc(u32 col_row, char c, u32 fg, u32 bg) {
    SVC4(SVC_FB_PUTC, col_row, (u32) c, fg, bg);
}

// ─── RNG ─────────────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline u32 sys_rand(void) {
    return SVC0(SVC_RAND);
}
[[maybe_unused]]
static inline void sys_rand_seed(u32 seed) {
    SVC1(SVC_RAND_SEED, seed);
}

// ─── Debug ───────────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline u32 sys_uart_write(const char *buf, u32 len) {
    return SVC2(SVC_UART_WRITE, buf, len);
}

// ─── Block device ─────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline bool sys_blk_read(u64 sector, u32 count, void *buf) {
    return (bool) SVC4(SVC_BLK_READ, (u32) sector, (u32) (sector >> 32), count, buf);
}
[[maybe_unused]]
static inline bool sys_blk_write(u64 sector, u32 count, const void *buf) {
    return (bool) SVC4(SVC_BLK_WRITE, (u32) sector, (u32) (sector >> 32), count, buf);
}

// ─── Filesystem ──────────────────────────────────────────────────────────────

[[maybe_unused]]
static inline u32 sys_fs_blob_count(void) {
    return SVC0(SVC_FS_BLOB_COUNT);
}

// Returns name length, or 0 if index is out of range.
// name_buf receives a null-terminated name; size_out receives blob data size.
[[maybe_unused]]
static inline u32 sys_fs_blob_info(u32 index, char *name_buf, u32 name_buf_size, u32 *size_out) {
    return SVC4(SVC_FS_BLOB_INFO, index, name_buf, name_buf_size, size_out);
}

#endif // SYSCALL_H
