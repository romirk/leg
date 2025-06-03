#include "boot.h"

#include "linker.h"
#include "types.h"
#include "uart.h"

[[clang::no_builtin]]
[[gnu::section(".startup")]]
void kboot() {
    // copy binary to RAM
    const auto len = kernel_main_end - kernel_main_off;
    const auto start = kernel_load_off;
    for (auto i = 0; i < len; i++) {
        kernel_main_off[i] = start[i];
    }

    // jump
    asm("ldr pc, =kmain");
}

// exceptions from delft os ---------------------------------------------------

[[gnu::used]]
[[gnu::section(".startup")]]
void handle_boot_exception(void) {
    *UART0_BASE = '!';
    *UART0_BASE = '\n';
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
