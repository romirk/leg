#include "kernel/kmain.h"

#include "kernel/dev/blk.h"
#include "kernel/dev/fb.h"
#include "kernel/dev/fwcfg.h"
#include "kernel/dev/gic.h"
#include "kernel/dev/kbd.h"
#include "kernel/dev/rng.h"
#include "kernel/dev/rtc.h"
#include "kernel/dev/uart.h"
#include "kernel/dtb.h"
#include "kernel/exceptions.h"
#include "kernel/fs.h"
#include "kernel/linker.h"
#include "kernel/logs.h"
#include "kernel/mem/alloc.h"
#include "kernel/mem/bump.h"
#include "kernel/process.h"
#include "kernel/scheduler.h"
#include "kernel/scheduler.h"
#include "libc/bswap.h"
#include "types.h"
#include "usr/main.h"
#include "utils.h"

#define EARLY_HEAP_SIZE 0x20000 // 128KB

[[noreturn, gnu::used]]
void kmain(void *dtb) {
    uart_puts("kernel: booting...\n");

    auto  header    = (struct dtb_header *) dtb;
    u32   dtb_size  = bswap32(header->totalsize);
    void *heap_base = align((u8 *) dtb + dtb_size, 16);
    early_malloc_init(heap_base, EARLY_HEAP_SIZE);

    dtb_tree_t tree;
    const auto dtb_err = dtb_parse(dtb, dtb_size, &tree, early_malloc);
    if (dtb_err != DTB_OK) {
        err("Failed to parse DTB: %d", dtb_err);
        goto halt;
    }

    dtb_node_t *chosen = dtb_find_node(&tree, "/chosen");
    if (chosen) {
        const dtb_prop_t *seed_prop = dtb_get_prop(chosen, "rng-seed");
        if (seed_prop && seed_prop->len >= 4) {
            u32 seed = 0;
            for (u32 i = 0; i + 4 <= seed_prop->len; i += 4)
                seed ^= be32_read(seed_prop->data + i);
            sys_rng_seed(seed);
        }
    }

    if (tree.memory_count == 0) {
        err("No memory nodes found in DTB");
        goto halt;
    }
    info("RAM: %p +%p", (void *) tree.memory[0].base, (void *) tree.memory[0].size);

    const u32 reserved_end = (u32) heap_base + EARLY_HEAP_SIZE;
    void     *kheap_va     = (void *) align_up((uptr) bss_end, PAGE_SIZE);
    mm_init((u32) tree.memory[0].base, tree.memory[0].size, reserved_end, kheap_va);
    early_malloc_reset();

    fwcfg_init();
    fb_init();

    info("kernel: started; DTB at %p (size %u bytes)", dtb, dtb_size);
    info("kernel: initialized framebuffer");

    struct pl011 uart;
    pl011_setup(&uart, 24000000u);
    info("UART: initialized");

    blk_init();
    if (!fs_mount(0)) {
        err("kernel: failed to mount filesystem");
        goto halt;
    }

    kbd_init();
    info("kbd: initialized");

    if (kbd_irq) gic_enable_irq(kbd_irq);

    gic_dist_init();
    gic_cpu_init();
    gic_enable_irq(UART_IRQ);
    gic_enable_irq(TIMER_IRQ);
    info("GIC: initialized");

    enable_interrupts();

#ifndef DEBUG
    timer_set_tick(300000, sched_tick); // 300ms tick

    fb_print("\n\nWelcome to ", FB_WHITE, FB_BLACK);
    fb_print("<uhhhhhhh>", FB_BLUE, FB_BLACK);
    fb_print("!\n\n\n", FB_WHITE, FB_BLACK);

    info("kernel: creating init process...");
    struct process *p = process_create("init");
    if (!p) {
        err("failed to create init process");
        goto halt;
    }
    process_exec(p);
#else
    info("kernel: debug main");
    main();
#endif

halt:
    warn("HALT");
    poweroff();
}
