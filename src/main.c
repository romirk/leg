//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"


#include "builtins.h"
#include "linker.h"

#include "cpu.h"
#include "logs.h"
#include "stdio.h"
#include "utils.h"
#include "fdt/parser.h"

[[noreturn]]
void kmain() {
    auto cpsr = get_cpsr();
    printf("Interrupts masked: %t\n"
           "Fast interrupts masked: %t\n"
           "Aborts masked: %t\n"
           "",
           cpsr.I, cpsr.F, cpsr.A);

    auto result = parse_fdt();

    info("memory\n\taddr: 0x%x\n\tsize: 0x%x\n", result.addr, result.size);

    warn("HALT");
    limbo();
}
