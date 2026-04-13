//
// Created by Romir Kulshrestha on 10/04/2026.
//

#include "kernel/dev/virtio.h"

#include "kernel/dev/memory.h"
#include "kernel/logs.h"

u32 virtio_find_device(const u32 device_id, u32 *const out_irq) {
    for (u32 i = 0; i < VIRTIO_MMIO_COUNT; i++) {
        const u32 base = VIRTIO_MMIO_BASE + i * VIRTIO_MMIO_STRIDE;
        if (VMIO(base, VMIO_MAGIC) != VIRTIO_MAGIC) continue;
        const u32 ver = VMIO(base, VMIO_VERSION);
        const u32 dev = VMIO(base, VMIO_DEVICE_ID);
        if (dev != 0) dbg("virtio: slot %d version=%d device_id=%d", i, ver, dev);
        if (dev != device_id) continue;
        if (out_irq) *out_irq = VIRTIO_MMIO_IRQ_BASE + i;
        return base;
    }
    return 0;
}

void virtio_init(void) {
}