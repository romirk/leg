//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"

#include "cpu.h"
#include "exceptions.h"
#include "linker.h"
#include "logs.h"
#include "stdio.h"
#include "uart.h"
#include "utils.h"
#include "fdt/parser.h"

[[noreturn]]
void kmain() {
    disable_interrupts();
    setup_exceptions();
    auto cpsr = read_cpsr();
    info("Interrupts masked: %t\n"
           "Fast interrupts masked: %t\n"
           "Aborts masked: %t",
           cpsr.I, cpsr.F, cpsr.A);
    auto sctlr = read_sctlr();
    info("VE: %t", sctlr.VE);
    auto periphbase = read_periphbase_39_15();
    info("Periphbase: %x", periphbase);

    warn("HALT");
    limbo();
}
