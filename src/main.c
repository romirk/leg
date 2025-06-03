//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"

#include "logs.h"
#include "stdio.h"
#include "fdt/fdt.h"
#include "fdt/parser.h"

[[noreturn]]
void kmain() {
    auto header = (struct fdt_header *) FDT_ADDR;
    fdt_endianness_swap(header);

    if (header->magic != FDT_MAGIC) {
        println("Not a valid FDT header!");
        goto limbo;
    }

    auto result = parse_fdt(header);

    log("memory\n\taddr: 0x%x\n\tsize: 0x%x\n", result.addr, result.size);

limbo:
    warn("HALT");
    for (;;) {
        asm("wfi");
    }
}
