#include "kernel/exceptions.h"

#include "kernel/cpu.h"
#include "kernel/gic.h"
#include "kernel/logs.h"
#include "kernel/rtc.h"
#include "kernel/uart.h"
#include "types.h"

[[gnu::section(".startup.exceptions")]]
void handle_boot_exception(void) {
    *UARTDR = '!';
    *UARTDR = '\n';
    asm("b #0"); // reboot
}

[[gnu::interrupt("ABORT")]]
void handle_data_abort(void) {
    *UARTDR = 'A';
}

[[gnu::interrupt("IRQ")]]
void handle_irq(void) {
    const u32 id = GICC_IAR & 0x3FFu;

    if (id == TIMER_IRQ) {
        rtc_timer_fired();
    } else if (id == UART_IRQ) {
        uart_irq_handler();
    }

    GICC_EOIR = id;
}

[[gnu::interrupt("FIQ")]]
void handle_fiq(void) {
    *UARTDR = '?';
}

[[gnu::interrupt("SWI")]]
void handle_svc(int svc_num) {
    u32 svc_instruction;

    asm volatile("ldr %0, [lr, #-4]"
                 : "=r"(svc_instruction)); // volatile: keeps asm from being eliminated

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
