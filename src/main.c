//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"

#include "cpu.h"
#include "exceptions.h"
#include "logs.h"
#include "stdio.h"
#include "uart.h"
#include "utils.h"
#include "fdt/parser.h"

[[noreturn]]
void kmain() {
    disable_interrupts();
    setup_exceptions();

    auto result = parse_fdt();
    info("memory start: 0x%p | memory size: 0x%x", result.addr, result.size);

    auto cpsr = read_cpsr();
    info("Interrupts masked: %t\n"
           "Fast interrupts masked: %t\n"
           "Aborts masked: %t",
           cpsr.I, cpsr.F, cpsr.A);
    auto sctlr = read_sctlr();
    info("VE: %t", sctlr.VE);

    warn("HALT");
    limbo();
}
