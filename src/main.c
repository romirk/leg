//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "main.h"

#include "cpu.h"
#include "exceptions.h"
#include "linker.h"
#include "logs.h"
#include "stdio.h"
#include "utils.h"
#include "fdt/parser.h"

[[noreturn]]
void kmain() {
    // auto cpsr = read_cpsr();
    // printf("Interrupts masked: %t\n"
    //        "Fast interrupts masked: %t\n"
    //        "Aborts masked: %t\n",
    //        cpsr.I, cpsr.F, cpsr.A);

    // write vtable address to vbar
    struct vbar vbar = {.addr = (u32) kernel_main_beg >> 5};
    write_vbar(vbar);

    info("svc: %x", svc_called);
    asm ("svc #0x42");
    info("svc: %x", svc_called);

    auto result = parse_fdt();

    info("memory\n\taddr: 0x%x\n\tsize: 0x%x\n", result.addr, result.size);

    warn("HALT");
    limbo();
}
