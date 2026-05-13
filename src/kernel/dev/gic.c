#include "kernel/dev/gic.h"

void gic_enable_irq(u32 irq) {
    u32 reg             = irq / 32u;
    u32 bit             = 1u << (irq % 32u);
    GICD_ISENABLER(reg) = bit;

    // route to CPU0
    u32 shift = (irq % 4u) * 8u;
    u32 cur   = GICD_ITARGETSR(irq);
    cur &= ~(0xFFu << shift);
    cur |= (0x01u << shift);
    GICD_ITARGETSR(irq) = cur;

    GICD_IPRIORITYR(irq) = 0xA0u;
}
