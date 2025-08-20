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
    main_fn *main_ptr = &main;

    dbg("Jumping to main@0x%p", main_ptr);

    // TODO give main_ptr to a scheduler
    auto ret = main();
    if (ret) {
        err("Main process exited with code %d", ret);
    }

    warn("HALT");
    limbo;
}
