//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"

#include "stdio.h"
#include "fdt/fdt.h"
#include "fdt/parser.h"

[[noreturn]]
void kmain() {
    err("errors enabled");
    warn("warnings enabled");
    log("logging enabled");
    dbg("debugs enabled");

    auto header = (struct fdt_header *) FDT_ADDR;
    fdt_endianness_swap(header);

    if (header->magic != FDT_MAGIC) {
        println("Not a valid FDT header!");
        goto limbo;
    }

    auto result = parse_fdt(header);

    printf("memory:\n\taddr: 0x%x\n\tsize: 0x%x\n", result.addr, result.size);

limbo:
    println("HALT");
    for (;;) {
        asm("wfi");
    }
}
