//
// Created by Romir Kulshrestha on 04/06/2025.
//

#include "exceptions.h"

#include "cpu.h"
#include "linker.h"
#include "uart.h"
#include "types.h"

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

[[gnu::interrupt("IRQ")]]
void handle_irq(void) {
    *UARTDR = '!';
}

[[gnu::interrupt("FIQ")]]
void handle_fiq(void) {
}

[[gnu::interrupt("SWI")]]
void handle_svc(void) {
    u32 svc_instruction;
    asm("ldr %0, [lr, #-4]" : "=r"(svc_instruction));
    // const auto svc = svc_instruction & 0xffffff;
}

void setup_exceptions(void) {
    // write vtable address to vbar
    struct vbar vbar = {.addr = (u32) kernel_main_beg >> 5};
    write_vbar(vbar);
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
