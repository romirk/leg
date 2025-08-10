//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"

#include "exceptions.h"
#include "logs.h"
#include "memory.h"
#include "stdio.h"
#include "stdlib.h"
#include "utils.h"
#include "fdt/parser.h"

[[noreturn]]
[[gnu::used]]
void kmain() {
    disable_interrupts();

    auto result = parse_fdt();
    info("memory start: 0x%p | memory size: 0x%x", result.addr, result.size);

    warn("HALT");
    limbo();
}
