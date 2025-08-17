//
// Created by Romir Kulshrestha on 04/06/2025.
//

#include "exceptions.h"

#include "cpu.h"
#include "logs.h"
#include "uart.h"
#include "types.h"

[[gnu::section(".startup.exceptions")]]
void handle_boot_exception(void) {
    *UARTDR = '!';
    *UARTDR = '\n';
    asm("b #0"); // reboot
}

[[gnu::interrupt("ABORT")]]
void handle_page_fault(void) {
    // We got a page fault...
    *UARTDR = 'A';
    // We handled the page fault
}

[[gnu::interrupt("IRQ")]]
void handle_irq(void) {
    *UARTDR = '!';
}

[[gnu::interrupt("FIQ")]]
void handle_fiq(void) {
    *UARTDR = '?';
}

[[gnu::interrupt("SWI")]]
void handle_svc(int svc_num) {
    u32 svc_instruction;

    // NOTE: `VOLATILE` PREVENTS THIS INSTRUCTION FROM BEING RENDERED USELESS
    asm volatile ("ldr %0, [lr, #-4]" : "=r"(svc_instruction));

    warn("SVC %x (%x)", svc_instruction & 0xff, svc_num);
}

void enable_interrupts(void) {
    auto cpsr = read_cpsr();
    cpsr.I = false;
    cpsr.A = false;
    cpsr.F = false;
    write_cpsr(cpsr);
}

void disable_interrupts(void) {
    auto cpsr = read_cpsr();
    cpsr.I = true;
    cpsr.A = true;
    cpsr.F = true;
    write_cpsr(cpsr);
}
