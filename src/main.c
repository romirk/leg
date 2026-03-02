//
// Created by Romir Kulshrestha on 20/08/2025.
//

#include "main.h"

#include "exceptions.h"
#include "logs.h"
#include "gic.h"
#include "utils.h"



[[gnu::noinline]]
int main() {
    /* initialize GIC */
    gic_cpu_init();
    gic_dist_init();

    /* enable the timer IRQ found in device tree */
    gic_enable_irq(TIMER_IRQ);

    /* program timer for 1000000 us (1s) */
    timer_set_oneshot_us(1000000u);

    // /* enable IRQs globally */
    // asm volatile("cpsie i");
    // limbo;

    return 0;
}
