//
// Created by Romir Kulshrestha on 01/06/2025.
//

#include "kernel/kmain.h"

#include "kernel/bump.h"
#include "kernel/logs.h"
#include "kernel/mm.h"
#include "utils.h"
#include "bswap.h"

#include "kernel/exceptions.h"
#include "kernel/fb.h"
#include "kernel/fwcfg.h"
#include "kernel/gic.h"
#include "kernel/main.h"
#include "kernel/uart.h"
#include "kernel/fdt/fdt.h"
#include "kernel/fdt/dtb.h"

#define EARLY_HEAP_SIZE 0x20000 // 128KB

[[noreturn]]
[[gnu::used]]
void kmain(void *dtb) {
    // set up bump allocator after DTB blob
    auto header = (struct fdt_header *) dtb;
    u32 dtb_size = bswap32(header->totalsize);
    void *heap_base = align((u8 *) dtb + dtb_size, 16);
    early_malloc_init(heap_base, EARLY_HEAP_SIZE);

    // parse full device tree
    dtb_tree_t tree;
    const auto dtb_err = dtb_parse(dtb, dtb_size, &tree, early_malloc);
    if (dtb_err != DTB_OK) {
        err("Failed to parse DTB: %d", dtb_err);
        goto halt;
    }

    if (tree.memory_count == 0) {
        err("No memory nodes found in DTB");
        goto halt;
    }
    info("RAM: %p +%p", (u32) tree.memory[0].base, (u32) tree.memory[0].size);

    // init full page allocator + kernel heap
    const u32 reserved_end = (u32) heap_base + EARLY_HEAP_SIZE;
    mm_init((u32) tree.memory[0].base, tree.memory[0].size, reserved_end);
    early_malloc_reset();

    // init UART with RX interrupts
    struct pl011 uart;
    pl011_setup(&uart, 24000000u);

    // init framebuffer (ramfb via fw-cfg)
    fwcfg_init();
    fb_init();

    // init GIC
    gic_dist_init();
    gic_cpu_init();
    gic_enable_irq(UART_IRQ);
    gic_enable_irq(TIMER_IRQ);
    timer_set_oneshot_us(1000000u);

    // enable IRQs globally
    enable_interrupts();

    dbg("Jumping to main@0x%p", &main);

    // TODO give main_ptr to a scheduler
    auto ret = main();
    if (ret) {
        err("Main process exited with code %d", ret);
    }

halt:
    warn("HALT");
    poweroff();
}
