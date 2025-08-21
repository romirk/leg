//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "kmain.h"

#include "logs.h"
#include "utils.h"

#include "main.h"


[[noreturn]]
[[gnu::used]]
void kmain() {
    dbg("Jumping to main@0x%p", &main);
    info("%b", get_bits(0x00ff0000, 19, 12));

    // TODO give main_ptr to a scheduler
    auto ret = main();
    if (ret) {
        err("Main process exited with code %d", ret);
    }

    warn("HALT");
    limbo;
}
