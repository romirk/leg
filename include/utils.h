#ifndef UTILS_H
#define UTILS_H

#include "types.h"

#define loop  for (;;)
#define limbo loop asm("wfi")

#define get_bits(data, high, low)   (((u32) (data) & ~(0xffffffffu << ((high) + 1))) >> (low))
#define get_high_bits(data, n_bits) ((u32) (data) >> (32 - (n_bits)))

[[maybe_unused]]
static void swap(char *a, char *b) {
    const auto t = *a;
    *a = *b;
    *b = t;
}

[[gnu::const, maybe_unused]]
static void *align(void *ptr, const u8 alignment) {
    return (void *) (-alignment & (u32) ptr + alignment - 1);
}

[[gnu::const, maybe_unused]]
static inline uptr align_up(uptr x, uptr align) {
    return (x + align - 1u) & ~(align - 1u);
}
[[gnu::const, maybe_unused]]
static uptr align_down(uptr x, uptr align) {
    return x & ~(align - 1u);
}
[[gnu::const, maybe_unused]]
static u32 div_round_up(u32 a, u32 b) {
    return (a + b - 1u) / b;
}

[[noreturn, gnu::format(printf, 1, 2)]]
void panic(const char *fmt, ...);

#define ASSERT(cond, msg)                                                                          \
    do {                                                                                           \
        if (!__builtin_expect(!!(cond), 1)) panic(msg);                                            \
    } while (0)

// PSCI v0.2 via HVC (QEMU virt)
#define PSCI_SYSTEM_OFF   0x84000008u
#define PSCI_SYSTEM_RESET 0x84000009u

[[noreturn, maybe_unused]]
static void poweroff(void) {
    asm volatile("mov r0, %0\n\thvc #0" ::"r"(PSCI_SYSTEM_OFF) : "r0");
    __builtin_unreachable();
}

#endif // UTILS_H
