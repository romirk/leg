//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "kernel/kmain.h"

#include "kernel/bump.h"
#include "kernel/logs.h"
#include "kernel/mm.h"
#include "utils.h"
#include "libc/stdlib.h"

#include "kernel/main.h"
#include "kernel/fdt/fdt.h"
#include "kernel/fdt/dtb.h"

#define EARLY_HEAP_SIZE 0x20000 // 128KB

[[noreturn]]
[[gnu::used]]
void kmain(void *dtb) {
    // set up bump allocator after DTB blob
    auto header = (struct fdt_header *) dtb;
    u32 dtb_size = swap_endianness(header->totalsize);
    void *heap_base = align((u8 *) dtb + dtb_size, 16);
    early_malloc_init(heap_base, EARLY_HEAP_SIZE);

    // parse full device tree
    dtb_tree_t tree;
    auto dtb_err = dtb_parse(dtb, dtb_size, &tree, early_malloc);
    if (dtb_err != DTB_OK) {
        err("Failed to parse DTB: %d", dtb_err);
        goto halt;
    }

    if (tree.memory_count == 0) {
        err("No memory nodes found in DTB");
        goto halt;
    }
    info("RAM: %p +%p", (u32) tree.memory[0].base, (u32) tree.memory[0].size);
    dtb_dump(&tree);

    // init full page allocator + kernel heap
    u32 reserved_end = (u32) heap_base + EARLY_HEAP_SIZE;
    mm_init((u32) tree.memory[0].base, tree.memory[0].size, reserved_end);
    early_malloc_reset();

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
