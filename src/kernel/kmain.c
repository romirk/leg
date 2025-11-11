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
    parse_fdt();
    dbg("Parsed dtb");
    dbg("Jumping to main@0x%p", &main);

    // TODO give main_ptr to a scheduler
    auto ret = main();
    if (ret) {
        err("Main process exited with code %d", ret);
    }

    warn("HALT");
    limbo;
}
