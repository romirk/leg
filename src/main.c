//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"

#include "exceptions.h"
#include "logs.h"
#include "memory.h"
#include "stdio.h"
#include "utils.h"
#include "fdt/parser.h"

[[noreturn]]
[[gnu::used]]
void kmain() {
    disable_interrupts();

    init_mmu();

    auto result = parse_fdt();
    if (!result.success) {
        info("Failed to parse fdt");
    } else
        info("memory start: 0x%p | memory size: 0x%x", result.addr, result.size);

    warn("HALT");
    limbo;
}
