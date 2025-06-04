//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"

#include "logs.h"
#include "stdio.h"
#include "utils.h"
#include "fdt/fdt.h"
#include "fdt/parser.h"

[[noreturn]]
void kmain() {
    auto header = (struct fdt_header *) FDT_ADDR;

    auto result = parse_fdt(header);

    log("memory\n\taddr: 0x%x\n\tsize: 0x%x\n", result.addr, result.size);

    warn("HALT");
    limbo();
}
