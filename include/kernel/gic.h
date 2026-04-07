//
// Created by Romir Kulshrestha on 05/06/2025.
//

#ifndef GIC_H
#define GIC_H
#include "types.h"

/* GIC bases from device tree */
#define GICD_BASE   0x08000000u
#define GICC_BASE   0x08010000u

/* GIC Distributor registers */
#define GICD_CTLR           (*(volatile u32 *)(GICD_BASE + 0x000))
#define GICD_ISENABLER(n)   (*(volatile u32 *)(GICD_BASE + 0x100 + ((n) * 4)))
#define GICD_ITARGETSR(i)   (*(volatile u32 *)(GICD_BASE + 0x800 + ((i) & ~3)))
#define GICD_IPRIORITYR(i)  (*(volatile u32 *)(GICD_BASE + 0x400 + (i)))

/* GIC CPU Interface registers */
#define GICC_CTLR   (*(volatile u32 *)(GICC_BASE + 0x00))
#define GICC_PMR    (*(volatile u32 *)(GICC_BASE + 0x04))
#define GICC_IAR    (*(volatile u32 *)(GICC_BASE + 0x0C))
#define GICC_EOIR   (*(volatile u32 *)(GICC_BASE + 0x10))

/* ARM generic timer CNTP = PPI 14 = GIC IRQ 30 */
#define TIMER_IRQ   30u

/* --- GIC helpers --- */
inline void gic_cpu_init(void) {
    /* set priority mask to allow all priorities */
    GICC_PMR = 0xFFu;
    /* enable CPU interface */
    GICC_CTLR = 0x1u;
}

inline void gic_dist_init(void) {
    /* enable distributor */
    GICD_CTLR = 0x1u;
}

void gic_enable_irq(u32 irq);

void timer_set_oneshot_us(u32 usec);

void timer_advance_cval(u32 usec);

void timer_disable(void);

#endif //GIC_H
