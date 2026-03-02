//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "kmain.h"

#include "logs.h"
#include "utils.h"

#include "main.h"
#include "fdt/parser.h"


[[noreturn]]
[[gnu::used]]
void kmain() {
    // parse fdt
    auto parse_result = parse_fdt();
    if (!parse_result.is_ok) {
        if (parse_result.value)
            err("Failed to parse fdt: %s", parse_result.value);
        goto halt;
    }
    const auto fdt = *(struct fdt_parse_result *) parse_result.value;
    info("RAM: %p", fdt.size);

    dbg("Jumping to main@0x%p", &main);

    // TODO give main_ptr to a scheduler
    auto ret = main();
    if (ret) {
        err("Main process exited with code %d", ret);
    }

halt:
    warn("HALT");
    limbo;
}
