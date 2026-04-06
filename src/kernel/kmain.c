//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "kmain.h"

#include "bump.h"
#include "logs.h"
#include "utils.h"
#include "stdlib.h"

#include "main.h"
#include "fdt/fdt.h"
#include "fdt/parser.h"

#define EARLY_HEAP_SIZE 0x20000 // 128KB

[[noreturn]]
[[gnu::used]]
void kmain(void *dtb) {
    // set up bump allocator after DTB blob
    auto header = (struct fdt_header *) dtb;
    u32 dtb_size = swap_endianness(header->totalsize);
    void *heap_base = align((u8 *) dtb + dtb_size, 16);
    early_malloc_init(heap_base, EARLY_HEAP_SIZE);

    // parse fdt
    auto parse_result = parse_fdt(dtb);
    if (!parse_result.is_ok) {
        if (parse_result.value)
            err("Failed to parse fdt: %s", parse_result.value);
        goto halt;
    }
    info("RAM: %p", ((struct fdt_parse_result *) parse_result.value)->size);

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
