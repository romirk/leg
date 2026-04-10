//
// Created by Romir Kulshrestha on 10/04/2026.
//

#include "kernel/dev/virtio.h"

#include "kernel/logs.h"
#include "kernel/tty.h"
#include "kernel/dev/memory.h"
#include "kernel/dev/kbd.h"
#include "kernel/mem/alloc.h"

// Legacy virtqueue memory layout (single contiguous page-aligned block):
//   [0x000] descriptor table: QUEUE_SIZE * 16 = 256 bytes
//   [0x100] available ring:   4 + 2*QUEUE_SIZE = 36 bytes
//   [0x1000] used ring:       4 + 8*QUEUE_SIZE  (at PAGE_SIZE boundary = QueueAlign)
[[gnu::aligned(4096)]]
static u8 g_queue_mem[2 * PAGE_SIZE];

#define g_desc  ((vq_desc_t *) g_queue_mem)
#define g_avail ((vq_avail_t *) (g_queue_mem + QUEUE_SIZE * 16u))
#define g_used  ((vq_used_t *) (g_queue_mem + PAGE_SIZE))

static virtio_input_event_t g_events[QUEUE_SIZE];
static u16                  g_last_used_idx;
static u32                  g_virtio_base;

static u32 find_keyboard(void) {
    for (u32 i = 0; i < VIRTIO_MMIO_COUNT; i++) {
        const u32 base = VIRTIO_MMIO_BASE + i * VIRTIO_MMIO_STRIDE;
        if (VMIO(base, VMIO_MAGIC) != VIRTIO_MAGIC) continue;
        const u32 ver = VMIO(base, VMIO_VERSION);
        const u32 dev = VMIO(base, VMIO_DEVICE_ID);
        if (dev != 0) dbg("kbd: slot %d version=%d device_id=%d", i, ver, dev);
        if (dev != VIRTIO_DEVICE_INPUT) continue;
        kbd_irq = VIRTIO_MMIO_IRQ_BASE + i;
        return base;
    }
    return 0;
}

void setup_eventq(u32 base) {
    for (u32 i = 0; i < QUEUE_SIZE; i++) {
        g_desc[i] = (vq_desc_t) {
            .addr = virt_to_phys(&g_events[i]),
            .len = sizeof(virtio_input_event_t),
            .flags = VDESC_F_WRITE,
        };
        g_avail->ring[i] = (u16) i;
    }
    g_avail->flags = 0;
    g_avail->idx = QUEUE_SIZE;
    g_used->flags = 0;
    g_used->idx = 0;

    VMIO(base, VMIO_GUEST_PAGE_SIZE) = PAGE_SIZE;
    VMIO(base, VMIO_QUEUE_SEL) = 0;
    const u32 qmax = VMIO(base, VMIO_QUEUE_NUM_MAX);
    VMIO(base, VMIO_QUEUE_NUM) = qmax < QUEUE_SIZE ? qmax : QUEUE_SIZE;
    VMIO(base, VMIO_QUEUE_ALIGN) = PAGE_SIZE;
    VMIO(base, VMIO_QUEUE_PFN) = virt_to_phys(g_queue_mem) >> 12;
}

void virtio_irq_handler(void) {
    u32 isr = VMIO(g_virtio_base, VMIO_INT_STATUS);
    VMIO(g_virtio_base, VMIO_INT_ACK) = isr;
    asm volatile("dsb" ::: "memory");

    while (g_last_used_idx != g_used->idx) {
        u32                   desc_id = g_used->ring[g_last_used_idx % QUEUE_SIZE].id;
        virtio_input_event_t *ev = &g_events[desc_id];

        handle_kbd_event(ev);

        g_avail->ring[g_avail->idx % QUEUE_SIZE] = (u16) desc_id;
        g_avail->idx++;
        g_last_used_idx++;
    }

    asm volatile("dsb" ::: "memory");
    VMIO(g_virtio_base, VMIO_QUEUE_NOTIFY) = 0;
}

void kbd_init(void) {
    u32 base = find_keyboard();
    if (!base) {
        warn("kbd: no virtio-input device found");
        return;
    }
    info("kbd: found at base=0x%x irq=%d", base, kbd_irq);
    g_virtio_base = base;

    VMIO(base, VMIO_STATUS) = 0;
    VMIO(base, VMIO_STATUS) = VSTAT_ACKNOWLEDGE | VSTAT_DRIVER;

    VMIO(base, VMIO_GUEST_FEAT_SEL) = 0;
    VMIO(base, VMIO_GUEST_FEAT) = 0;

    setup_eventq(base);
    VMIO(base, VMIO_STATUS) = VSTAT_ACKNOWLEDGE | VSTAT_DRIVER | VSTAT_DRIVER_OK;
    info("kbd: ready, status=0x%x", VMIO(base, VMIO_STATUS));

    asm volatile("dsb" ::: "memory");
    VMIO(base, VMIO_QUEUE_NOTIFY) = 0;
}

void virtio_init(void) {
}