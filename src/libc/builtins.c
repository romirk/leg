#include "libc/builtins.h"
#include "types.h"

void *memcpy(void *dst, const void *src, size_t len) {
    if (dst == src) return nullptr;
    u8       *dp8 = dst;
    const u8 *sp8 = (void *) src;
    while (len--)
        *dp8++ = *sp8++;

    return dst;
}

static void *revcpy(void *dst, const void *src, size_t len) {
    if (dst == src) return nullptr;
    u8       *dp8 = dst + len - 1;
    const u8 *sp8 = src + len - 1;
    while (len--)
        *dp8-- = *sp8--;

    return dst;
}

void *memmove(void *dst, const void *src, const size_t len) {
    if (dst < src) return memcpy(dst, src, len);
    if (dst > src) return revcpy(dst, src, len);
    return dst;
}

void *memset(void *dst, const int c, size_t len) {
    u8 *dp = dst;
    while (len--)
        *dp++ = c;
    return dst;
}

void *memclr(void *dst, const size_t len) {
    return memset(dst, 0, len);
}

// ARM EABI 64-bit unsigned divide+modulo.
// Inputs:  r0:r1 = numerator (lo:hi), r2:r3 = denominator (lo:hi)
// Outputs: r0:r1 = quotient,           r2:r3 = remainder
// Uses binary long division (64 iterations).
[[gnu::naked]]
void __aeabi_uldivmod(void) {
    asm(
        "push  {r4-r9, lr}       \n" // save callee-saved + lr
        "mov   r4, #0            \n" // q_lo  = 0
        "mov   r5, #0            \n" // q_hi  = 0
        "mov   r6, #0            \n" // rem_lo = 0
        "mov   r7, #0            \n" // rem_hi = 0
        "mov   r8, #64           \n" // loop counter

        "1:                      \n"
        // r9 = MSB of n (bit 31 of n_hi = r1)
        "mov   r9, r1, lsr #31   \n"
        // n <<= 1 (64-bit): bring top bit of n_lo into bottom of n_hi
        "mov   r1, r1, lsl #1    \n"
        "orr   r1, r1, r0, lsr #31\n"
        "mov   r0, r0, lsl #1    \n"
        // rem = (rem << 1) | r9
        "mov   r7, r7, lsl #1    \n"
        "orr   r7, r7, r6, lsr #31\n"
        "mov   r6, r6, lsl #1    \n"
        "orr   r6, r6, r9        \n"
        // q <<= 1
        "mov   r5, r5, lsl #1    \n"
        "orr   r5, r5, r4, lsr #31\n"
        "mov   r4, r4, lsl #1    \n"
        // if rem >= d: rem -= d, q |= 1
        "cmp   r7, r3            \n" // compare rem_hi : d_hi
        "bhi   2f                \n" // rem_hi > d_hi → subtract
        "blo   3f                \n" // rem_hi < d_hi → skip
        "cmp   r6, r2            \n" // rem_hi == d_hi: compare lo
        "blo   3f                \n"
        "2:                      \n"
        "subs  r6, r6, r2        \n"
        "sbc   r7, r7, r3        \n"
        "orr   r4, r4, #1        \n" // set bit 0 of quotient
        "3:                      \n"
        "subs  r8, r8, #1        \n"
        "bne   1b                \n"
        // move results to output registers
        "mov   r2, r6            \n" // rem_lo → r2
        "mov   r3, r7            \n" // rem_hi → r3
        "mov   r0, r4            \n" // q_lo   → r0
        "mov   r1, r5            \n" // q_hi   → r1
        "pop   {r4-r9, pc}       \n"
    );
}
