//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"

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
    print_num(result.mem_count);
    println(" memory reg(s)");
    for (auto i = 0u; i < result.mem_count; i++) {
        auto reg = result.regs[i];
        u32 addr = ((u64) reg.addr[0] << 32 | reg.addr[1]) & 0xffffffff;
        u32 size = ((u64) reg.size[0] << 32 | reg.size[1]) & 0xffffffff;
        print("addr: 0x");
        print_ptr((void *) addr);
        print(" size: 0x");
        print_ptr((void *) size);
        putchar('\n');
    }
limbo:
    println("HALT");
    for (;;) {
        asm("wfi");
    }
}
