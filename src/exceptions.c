//
// Created by Romir Kulshrestha on 04/06/2025.
//

#include "exceptions.h"

#include "uart.h"
#include "types.h"

int svc_called = -1;

// exceptions from delft os ---------------------------------------------------

[[gnu::used]]
[[gnu::section(".startup.c")]]
void handle_boot_exception(void) {
    *UARTDR = '!';
    *UARTDR = '\n';
    asm("b #0"); // reboot
}

[[gnu::interrupt("ABORT")]]
void handle_page_fault(void) {
    // We got a page fault...
    asm volatile ("nop");
    // We handled the page fault
}

/**
 * IRQ handler during the boot process
 */
[[gnu::interrupt("IRQ")]]
void handle_irq(void) {
    // No idea how to handle an IRQ during boot. Probably just ignore it.
    // TODO: figure out how to clear IRQ and continue booting.
}

[[gnu::interrupt("FIQ")]]
void handle_fiq(void) {
    // TODO: figure out if we need this during boot
    // You can select exactly one interrupt to be a fast-interrupt.
    // To do this, set the FIQ register according to the manual
    // (BCM2835 ARM Peripherals, 7.5 Interrupts, FIQ Register).
}

[[gnu::interrupt("SWI")]]
void handle_svc(void) {
    u32 svc;
    asm("ldr %0, [lr, #-4]" : "=r"(svc));
    svc_called = (int) svc & 0xff;
}
