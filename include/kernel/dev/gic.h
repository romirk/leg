#ifndef GIC_H
#define GIC_H

#include "types.h"
#include "memory.h"

#define GICD_CTLR          (*(volatile u32 *) (GICD_BASE + 0x000))
#define GICD_ISENABLER(n)  (*(volatile u32 *) (GICD_BASE + 0x100 + ((n) * 4)))
#define GICD_ITARGETSR(i)  (*(volatile u32 *) (GICD_BASE + 0x800 + ((i) & ~3)))
#define GICD_IPRIORITYR(i) (*(volatile u32 *) (GICD_BASE + 0x400 + (i)))

#define GICC_CTLR (*(volatile u32 *) (GICC_BASE + 0x00))
#define GICC_PMR  (*(volatile u32 *) (GICC_BASE + 0x04))
#define GICC_IAR  (*(volatile u32 *) (GICC_BASE + 0x0C))
#define GICC_EOIR (*(volatile u32 *) (GICC_BASE + 0x10))

// CNTP = PPI 14 = GIC IRQ 30
#define TIMER_IRQ 30u

[[maybe_unused]]
static void gic_cpu_init(void) {
    GICC_PMR = 0xFFu; // allow all priorities
    GICC_CTLR = 0x1u; // enable CPU interface
}

[[maybe_unused]]
static void gic_dist_init(void) {
    GICD_CTLR = 0x1u; // enable distributor
}

void gic_enable_irq(u32 irq);

void timer_set_oneshot_us(u32 usec);

void timer_advance_cval(u32 usec);

void timer_disable(void);

#endif // GIC_H
