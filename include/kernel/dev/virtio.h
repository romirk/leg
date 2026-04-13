//
// Created by Romir Kulshrestha on 10/04/2026.
//

#ifndef LEG_VIRTIO_H
#define LEG_VIRTIO_H

#include "types.h"

#define VIRTIO_MMIO_STRIDE   0x200u
#define VIRTIO_MMIO_COUNT    32u
#define VIRTIO_MMIO_IRQ_BASE 48u

#define VIRTIO_MAGIC        0x74726976u
#define VIRTIO_DEVICE_BLOCK 2u
#define VIRTIO_DEVICE_INPUT 18u

// legacy (v1) register offsets
#define VMIO_MAGIC           0x000
#define VMIO_VERSION         0x004
#define VMIO_DEVICE_ID       0x008
#define VMIO_GUEST_FEAT      0x020
#define VMIO_GUEST_FEAT_SEL  0x024
#define VMIO_GUEST_PAGE_SIZE 0x028
#define VMIO_QUEUE_SEL       0x030
#define VMIO_QUEUE_NUM_MAX   0x034
#define VMIO_QUEUE_NUM       0x038
#define VMIO_QUEUE_ALIGN     0x03c
#define VMIO_QUEUE_PFN       0x040
#define VMIO_QUEUE_NOTIFY    0x050
#define VMIO_INT_STATUS      0x060
#define VMIO_INT_ACK         0x064
#define VMIO_STATUS          0x070

#define VMIO(base, off) (*(volatile u32 *) ((base) + (off)))

#define VSTAT_ACKNOWLEDGE 1u
#define VSTAT_DRIVER      2u
#define VSTAT_DRIVER_OK   4u

#define VDESC_F_NEXT  1u // descriptor chains to .next
#define VDESC_F_WRITE 2u // device writes into buffer

#define QUEUE_SIZE 16u

typedef struct {
    u64 addr;
    u32 len;
    u16 flags;
    u16 next;
} vq_desc_t;

typedef struct {
    u16 flags;
    u16 idx;
    u16 ring[QUEUE_SIZE];
} vq_avail_t;

typedef struct {
    u32 id;
    u32 len;
} vq_used_elem_t;

typedef struct {
    u16            flags;
    u16            idx;
    vq_used_elem_t ring[QUEUE_SIZE];
} vq_used_t;

typedef struct {
    u16 type;
    u16 code;
    u32 value;
} virtio_input_event_t;

// Returns the MMIO base of the first virtio device matching device_id,
// sets *out_irq to its GIC IRQ number, or returns 0 if not found.
u32  virtio_find_device(u32 device_id, u32 *out_irq);
void virtio_init(void);

#endif // LEG_VIRTIO_H
