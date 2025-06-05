//
// Created by Romir Kulshrestha on 04/06/2025.
//

#include "cpu.h"

struct cpsr read_cpsr() {
    struct cpsr cpsr;
    asm ("mrs %0, cpsr" : "=r"(cpsr));
    return cpsr;
}

[[gnu::naked]]
struct scr read_scr() {
    asm ("mrc p15, 0, r0, c1, c1, 0");
    asm ("bx lr");
}

struct vbar read_vbar() {
    struct vbar vbar;
    asm ("mrc p15, 0, %0, c12, c0, 0" : "=r"(vbar));
    return vbar;
}

struct sctlr read_sctlr(void) {
    struct sctlr sctlr;
    asm ("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    return sctlr;
}

u32 read_periphbase_39_15(void) {
    struct cbar cbar;
    asm ("MRC p15, 4, %0, c15, c0, 0" : "=r"(cbar));
    return cbar.PERIPHBASE_31_15 | cbar.PERIPHBASE_39_32 << 17;
}

void write_cpsr(struct cpsr cpsr) {
    asm ("msr cpsr, %0":: "r"(cpsr));
}

void write_vbar(struct vbar vbar) {
    asm ("mcr p15, 0, %0, c12, c0, 0" :: "r"(vbar));
}

void write_sctlr(struct sctlr sctlr) {
    asm ("mcr p15, 0, %0, c1, c0, 0" :: "r"(sctlr));
}
