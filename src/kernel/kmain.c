#include "kernel/kmain.h"

#include "kernel/mem/bump.h"
#include "kernel/logs.h"
#include "kernel/mem/alloc.h"
#include "utils.h"
#include "bswap.h"

#include "kernel/exceptions.h"
#include "kernel/dev/fb.h"
#include "kernel/dev/fwcfg.h"
#include "kernel/dev/gic.h"
#include "kernel/main.h"
#include "kernel/process.h"
#include "kernel/dev/kbd.h"
#include "kernel/dev/uart.h"
#include "kernel/fdt.h"
#include "kernel/dtb.h"

#define EARLY_HEAP_SIZE 0x20000 // 128KB

[[noreturn, gnu::used]]
void kmain(void *dtb) {
    auto  header = (struct fdt_header *) dtb;
    u32   dtb_size = bswap32(header->totalsize);
    void *heap_base = align((u8 *) dtb + dtb_size, 16);
    early_malloc_init(heap_base, EARLY_HEAP_SIZE);

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

    const u32 reserved_end = (u32) heap_base + EARLY_HEAP_SIZE;
    mm_init((u32) tree.memory[0].base, tree.memory[0].size, reserved_end);
    early_malloc_reset();

    struct pl011 uart;
    pl011_setup(&uart, 24000000u);
    kbd_init();
    if (kbd_irq) gic_enable_irq(kbd_irq);

    fwcfg_init();
    fb_init();

    gic_dist_init();
    gic_cpu_init();
    gic_enable_irq(UART_IRQ);
    gic_enable_irq(TIMER_IRQ);

    enable_interrupts();

    struct process *p = process_create(main);
    if (!p) {
        err("failed to create main process");
        goto halt;
    }
    process_exec(p);

halt:
    warn("HALT");
    poweroff();
}
