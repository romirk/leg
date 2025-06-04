//
// Created by Romir Kulshrestha on 04/06/2025.
//

#include "exceptions.h"

#include "uart.h"
#include "types.h"

// exceptions from delft os ---------------------------------------------------

[[gnu::used]]
[[gnu::section(".startup")]]
void handle_boot_exception(void) {
    *UARTDR = '!';
    *UARTDR = '\n';
    asm("b #0"); // reboot
}

[[gnu::used]]
[[gnu::interrupt("ABORT")]]
[[gnu::section(".startup")]]
void handle_page_fault_boot(void) {
    // We got a page fault...
    asm volatile ("nop");
    // We handled the page fault
}

/**
 * IRQ handler during the boot process
 */
[[gnu::used]]
[[gnu::interrupt("IRQ")]]
[[gnu::section(".startup")]]
void handle_irq_boot(void) {
    // No idea how to handle an IRQ during boot. Probably just ignore it.
    // TODO: figure out how to clear IRQ and continue booting.
}

[[gnu::used]]
[[gnu::interrupt("FIQ")]]
[[gnu::section(".startup")]]
void handle_fiq_boot(void) {
    // TODO: figure out if we need this during boot
    // You can select exactly one interrupt to be a fast-interrupt.
    // To do this, set the FIQ register according to the manual
    // (BCM2835 ARM Peripherals, 7.5 Interrupts, FIQ Register).
}
