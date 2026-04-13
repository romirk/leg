#include "syscall.h"

#include "kernel/cpu.h"
#include "kernel/dev/blk.h"
#include "kernel/dev/fb.h"
#include "kernel/dev/rng.h"
#include "kernel/dev/rtc.h"
#include "kernel/dev/uart.h"
#include "kernel/exceptions.h"
#include "kernel/logs.h"
#include "kernel/process.h"
#include "kernel/tty.h"
#include "types.h"

typedef u32 (*svc_handler_t)(u32 r0, u32 r1, u32 r2, u32 r3);

// ─── Process ─────────────────────────────────────────────────────────────────

static u32 svc_exit(u32 r0, [[maybe_unused]] u32 r1, [[maybe_unused]] u32 r2,
                    [[maybe_unused]] u32 r3) {
    process_exit(current_proc, (int) r0);
}

// ─── I/O ─────────────────────────────────────────────────────────────────────

static u32 svc_write(u32 r0, u32 r1, [[maybe_unused]] u32 r2, [[maybe_unused]] u32 r3) {
    const char *buf = (const char *) r0;
    for (u32 i = 0; i < r1; i++)
        tty_putchar(buf[i]);
    return r1;
}

static u32 svc_read(u32 r0, u32 r1, [[maybe_unused]] u32 r2, [[maybe_unused]] u32 r3) {
    enable_interrupts();
    u32 n = tty_readline((char *) r0, r1);
    disable_interrupts();
    return n;
}

static u32 svc_getchar([[maybe_unused]] u32 r0, [[maybe_unused]] u32 r1, [[maybe_unused]] u32 r2,
                       [[maybe_unused]] u32 r3) {
    enable_interrupts();
    char c = tty_getchar();
    disable_interrupts();
    return (u32) c;
}

static u32 svc_getchar_nb([[maybe_unused]] u32 r0, [[maybe_unused]] u32 r1, [[maybe_unused]] u32 r2,
                          [[maybe_unused]] u32 r3) {
    return (u32) tty_getchar_nb();
}

// ─── Timing ──────────────────────────────────────────────────────────────────

static u32 svc_sleep(u32 r0, [[maybe_unused]] u32 r1, [[maybe_unused]] u32 r2,
                     [[maybe_unused]] u32 r3) {
    enable_interrupts();
    delay_us(r0);
    disable_interrupts();
    return 0;
}

static u32 svc_ticks(u32 r0, [[maybe_unused]] u32 r1, [[maybe_unused]] u32 r2,
                     [[maybe_unused]] u32 r3) {
    *(u64 *) r0 = rtc_ticks();
    return 0;
}

static u32 svc_cntfrq([[maybe_unused]] u32 r0, [[maybe_unused]] u32 r1, [[maybe_unused]] u32 r2,
                      [[maybe_unused]] u32 r3) {
    return read_cntfrq();
}

// ─── Framebuffer ─────────────────────────────────────────────────────────────

static u32 svc_fb_clear(u32 r0, [[maybe_unused]] u32 r1, [[maybe_unused]] u32 r2,
                        [[maybe_unused]] u32 r3) {
    fb_clear(r0);
    return 0;
}

static u32 svc_fb_putpixel(u32 r0, u32 r1, u32 r2, [[maybe_unused]] u32 r3) {
    fb_putpixel(r0, r1, r2);
    return 0;
}

static u32 svc_fb_putc(u32 r0, u32 r1, u32 r2, u32 r3) {
    fb_putc_at(r0 >> 16, r0 & 0xFFFFu, (char) r1, r2, r3);
    return 0;
}

// ─── RNG ─────────────────────────────────────────────────────────────────────

static u32 svc_rand([[maybe_unused]] u32 r0, [[maybe_unused]] u32 r1, [[maybe_unused]] u32 r2,
                    [[maybe_unused]] u32 r3) {
    return sys_rand32();
}

static u32 svc_rand_seed(u32 r0, [[maybe_unused]] u32 r1, [[maybe_unused]] u32 r2,
                         [[maybe_unused]] u32 r3) {
    sys_rng_seed(r0);
    return 0;
}

// ─── Block device ────────────────────────────────────────────────────────────

static u32 svc_blk_read(u32 r0, u32 r1, u32 r2, u32 r3) {
    u64 sector = (u64) r0 | ((u64) r1 << 32);
    return blk_read(sector, r2, (void *) r3);
}

static u32 svc_blk_write(u32 r0, u32 r1, u32 r2, u32 r3) {
    u64 sector = (u64) r0 | ((u64) r1 << 32);
    return blk_write(sector, r2, (const void *) r3);
}

// ─── Debug ────────────────────────────────────────────────────────────────────

static u32 svc_uart_write(u32 r0, u32 r1, [[maybe_unused]] u32 r2, [[maybe_unused]] u32 r3) {
    const char *buf = (const char *) r0;
    for (u32 i = 0; i < r1; i++)
        uart_putchar(buf[i]);
    return r1;
}

// ─── Dispatch ────────────────────────────────────────────────────────────────

static const svc_handler_t svc_handlers[] = {
    [SVC_EXIT] = svc_exit,
    [SVC_WRITE] = svc_write,
    [SVC_READ] = svc_read,
    [SVC_GETCHAR] = svc_getchar,
    [SVC_GETCHAR_NB] = svc_getchar_nb,
    [SVC_SLEEP] = svc_sleep,
    [SVC_TICKS] = svc_ticks,
    [SVC_CNTFRQ] = svc_cntfrq,
    [SVC_FB_CLEAR] = svc_fb_clear,
    [SVC_FB_PUTPIXEL] = svc_fb_putpixel,
    [SVC_FB_PUTC] = svc_fb_putc,
    [SVC_RAND] = svc_rand,
    [SVC_RAND_SEED] = svc_rand_seed,
    [SVC_UART_WRITE] = svc_uart_write,
    [SVC_BLK_READ] = svc_blk_read,
    [SVC_BLK_WRITE] = svc_blk_write,
};
#define SVC_COUNT (sizeof(svc_handlers) / sizeof(*svc_handlers))

u32 svc_dispatch(u32 r0, u32 r1, u32 r2, u32 r3, u32 svc_num) {
    if (svc_num < SVC_COUNT && svc_handlers[svc_num]) return svc_handlers[svc_num](r0, r1, r2, r3);
    warn("unhandled SVC #%d", svc_num);
    return 0;
}
