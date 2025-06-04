//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"


#include "builtins.h"

#include "logs.h"
#include "stdio.h"
#include "uart.h"
#include "utils.h"
#include "math.h"
#include "fdt/parser.h"

[[noreturn]]
void kmain() {
    printf("log2(MAX) = %x", 2048);

    auto result = parse_fdt();
    // device tree is used, dont need it anymore
    // copy vtable to start of RAM, overwriting dtb
    memcpy((void *) 0x40000000, nullptr, 0x20);

    struct pl011 uart;
    pl011_setup(&uart, 24000000);

    info("memory\n\taddr: 0x%x\n\tsize: 0x%x\n", result.addr, result.size);

    warn("HALT");
    limbo();
}
