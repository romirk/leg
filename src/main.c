//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"

#include "stdio.h"
#include "uart.h"
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

    fdt_print_header(header);
    auto result = parse_fdt(header);
    auto reg = result.mem_regs[0];
    auto addr = reg.addr[1];
    auto size = reg.size[1];

    printf("memory:\n\taddr: 0x%x\n\tsize: 0x%x\n", addr, size);

limbo:
    println("HALT");
    for (;;) {
        asm("wfi");
    }
}
