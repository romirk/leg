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

void write_vbar(struct vbar vbar) {
    asm ("mcr p15, 0, %0, c12, c0, 0" :: "r"(vbar));
}
