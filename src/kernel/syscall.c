#include "kernel/exceptions.h"
#include "kernel/dev/rtc.h"
#include "kernel/logs.h"
#include "kernel/process.h"
#include "kernel/tty.h"
#include "syscall.h"
#include "types.h"

typedef u32 (*svc_handler_t)(u32 r0, u32 r1, u32 r2, u32 r3);

static u32 svc_exit(u32 r0, [[maybe_unused]] u32 r1, [[maybe_unused]] u32 r2,
                    [[maybe_unused]] u32 r3) {
    process_exit(current_proc, (int) r0);
}

static u32 svc_write(u32 r0, u32 r1, [[maybe_unused]] u32 r2, [[maybe_unused]] u32 r3) {
    const char *buf = (const char *) r0;
    for (u32 i = 0; i < r1; i++)
        putchar(buf[i]);
    return r1;
}

static u32 svc_read(u32 r0, u32 r1, [[maybe_unused]] u32 r2, [[maybe_unused]] u32 r3) {
    enable_interrupts(); // IRQs are masked on SVC entry; need them for kbd input
    u32 n = readline((char *) r0, r1);
    disable_interrupts();
    return n;
}

static u32 svc_sleep(u32 r0, [[maybe_unused]] u32 r1, [[maybe_unused]] u32 r2,
                     [[maybe_unused]] u32 r3) {
    enable_interrupts();
    delay_us(r0);
    disable_interrupts();
    return 0;
}

static const svc_handler_t svc_handlers[] = {
    [SVC_EXIT] = svc_exit,
    [SVC_WRITE] = svc_write,
    [SVC_READ] = svc_read,
    [SVC_SLEEP] = svc_sleep,
};
#define SVC_COUNT (sizeof(svc_handlers) / sizeof(*svc_handlers))

u32 svc_dispatch(u32 r0, u32 r1, u32 r2, u32 r3, u32 svc_num) {
    if (svc_num < SVC_COUNT && svc_handlers[svc_num]) return svc_handlers[svc_num](r0, r1, r2, r3);
    warn("unhandled SVC #%d", svc_num);
    return 0;
}