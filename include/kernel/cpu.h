// cpu.h — ARMv7 CP15 system register layouts and accessors

#ifndef CPU_H
#define CPU_H
#include "types.h"

// Configuration Base Address Register (CBAR) — GIC/peripheral base
struct [[gnu::packed]] cbar {
    u8 PERIPHBASE_39_32; // extended bits [39:32]
    u8 : 7;
    u32 PERIPHBASE_31_15 : 17; // base address bits [31:15]
};

// Generic timer control register (CNTP_CTL)
struct [[gnu::packed]] cntp_ctl {
    bool ENABLE : 1;  // timer active
    bool IMASK : 1;   // mask interrupt output
    bool ISTATUS : 1; // (read-only) condition met
    u32 : 29;
};

// CPSR M[4:0] — processor mode field values
enum processor_mode {
    usr = 0b10000,
    fiq = 0b10001,
    irq = 0b10010,
    svc = 0b10011,
    mon = 0b10110,
    abt = 0b10111,
    hyp = 0b11010,
    und = 0b11011,
    sys = 0b11111,
};

// Current Program Status Register (CPSR)
struct [[gnu::packed]] cpsr {
    u8   M : 5;   // processor mode (see processor_mode)
    bool T : 1;   // Thumb state
    bool F : 1;   // FIQ disable
    bool I : 1;   // IRQ disable
    bool A : 1;   // asynchronous abort disable
    bool E : 1;   // endianness (0 = little)
    u8   ITl : 6; // IT[7:2] — if-then block state (low)
    u8   GE : 4;  // SIMD >= condition flags
    u8 : 4;
    bool J : 1;   // Jazelle state
    u8   ITh : 2; // IT[1:0] — if-then block state (high)
    bool Q : 1;   // cumulative saturation
    bool V : 1;   // overflow
    bool C : 1;   // carry
    bool Z : 1;   // zero
    bool N : 1;   // negative
};

// Secure Configuration Register (SCR) — controls security state
struct [[gnu::packed]] scr {
    bool NS : 1;  // non-secure bit
    bool IRQ : 1; // IRQ handled in monitor mode
    bool FIQ : 1; // FIQ handled in monitor mode
    bool EA : 1;  // external abort taken to monitor mode
    bool FW : 1;  // FIQ enable write (secure)
    bool AW : 1;  // CPSR.A write (secure)
    bool nET : 1; // early termination disable
    bool SCD : 1; // SMC disable
    bool HCE : 1; // HVC enable
    bool SIF : 1; // secure instruction fetch
    u32 : 22;
};

// System Control Register (SCTLR) — MMU, caches, alignment, vectors
struct [[gnu::packed]] sctlr {
    bool M : 1; // MMU enable
    bool A : 1; // alignment fault enable
    bool C : 1; // data cache enable
    u8 : 2;
    bool CP15BEN : 1; // CP15 barrier enable
    bool : 1;
    bool B : 1; // big-endian (deprecated in v7)
    u8 : 2;
    bool SW : 1; // SWP/SWPB enable
    bool Z : 1;  // branch predictor enable
    bool I : 1;  // instruction cache enable
    bool V : 1;  // high vectors (0 = 0x00000000, 1 = 0xFFFF0000)
    bool RR : 1; // predictable cache replacement
    bool : 1;
    bool : 1;
    bool HA : 1; // hardware access flag enable
    bool : 1;
    bool WXN : 1;  // write implies XN
    bool UWXN : 1; // unprivileged write implies PL1 XN
    bool FI : 1;   // low-latency interrupts
    bool U : 1;    // (deprecated)
    bool : 1;
    bool VE : 1; // vectored interrupt enable
    bool EE : 1; // exception endianness
    bool : 1;
    bool NMFI : 1; // non-maskable FIQ
    bool TRE : 1;  // TEX remap enable
    bool AFE : 1;  // access flag enable
    bool TE : 1;   // Thumb exception enable
    bool : 1;
};

// Vector Base Address Register (VBAR) — base of the exception vector table
struct [[gnu::packed]] vbar {
    u8 : 5;
    u32 addr : 27; // bits [31:5] of vector table base
};

// Accessors — grouped by register (read then write)

[[maybe_unused]]
static u32 read_periphbase_39_15(void) {
    struct cbar cbar;
    asm("MRC p15, 4, %0, c15, c0, 0" : "=r"(cbar));
    return cbar.PERIPHBASE_31_15 | cbar.PERIPHBASE_39_32 << 17;
}

// CNTFRQ — counter frequency in Hz
[[maybe_unused]]
static u32 read_cntfrq(void) {
    u32 val;
    asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r"(val));
    return val;
}

// CNTPCT — free-running counter (64-bit)
[[maybe_unused]]
static u64 read_cntpct(void) {
    u32 lo, hi;
    asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(lo), "=r"(hi));
    return (u64) hi << 32 | lo;
}

// CNTP_CVAL — absolute compare value (64-bit); fires when CNTPCT >= CVAL
[[maybe_unused]]
static u64 read_cntp_cval(void) {
    u32 lo, hi;
    asm volatile("mrrc p15, 2, %0, %1, c14" : "=r"(lo), "=r"(hi));
    return (u64) hi << 32 | lo;
}

[[maybe_unused]]
static void write_cntp_cval(u64 val) {
    asm volatile("mcrr p15, 2, %Q0, %R0, c14" ::"r"(val));
}

[[maybe_unused]]
static void write_cntp_ctl(struct cntp_ctl val) {
    asm volatile("mcr p15, 0, %0, c14, c2, 1" ::"r"(val));
}

[[maybe_unused]]
static struct cpsr read_cpsr() {
    struct cpsr cpsr;
    asm("mrs %0, cpsr" : "=r"(cpsr));
    return cpsr;
}

[[maybe_unused]]
static void write_cpsr(struct cpsr cpsr) {
    asm("msr cpsr, %0" ::"r"(cpsr));
}

[[gnu::naked, maybe_unused]]
static struct scr read_scr() {
    asm("mrc p15, 0, r0, c1, c1, 0");
    asm("bx lr");
}

[[maybe_unused]]
static struct sctlr read_sctlr(void) {
    struct sctlr sctlr;
    asm("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    return sctlr;
}

[[maybe_unused]]
static void write_sctlr(struct sctlr sctlr) {
    asm("mcr p15, 0, %0, c1, c0, 0" ::"r"(sctlr));
}

[[maybe_unused]]
static struct vbar read_vbar() {
    struct vbar vbar;
    asm("mrc p15, 0, %0, c12, c0, 0" : "=r"(vbar));
    return vbar;
}

[[maybe_unused]]
static void write_vbar(struct vbar vbar) {
    asm("mcr p15, 0, %0, c12, c0, 0" ::"r"(vbar));
}

#endif // CPU_H