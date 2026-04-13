#include "kernel/dev/blk.h"

#include "kernel/dev/memory.h"
#include "kernel/dev/virtio.h"
#include "kernel/logs.h"
#include "kernel/mem/alloc.h"

#define VIRTIO_BLK_T_IN  0u
#define VIRTIO_BLK_T_OUT 1u

#define VIRTIO_BLK_S_OK 0u

typedef struct {
    u32 type;
    u32 reserved;
    u64 sector;
} virtio_blk_req_hdr_t;

u32        blk_irq;
static u32 g_blk_base;
static u16 g_last_used_idx;

// Legacy virtqueue memory layout — same as kbd:
//   [0x000] descriptor table: QUEUE_SIZE * 16
//   [0x100] available ring
//   [0x1000] used ring (PAGE_SIZE boundary = QueueAlign)
[[gnu::aligned(4096)]]
static u8 g_queue_mem[2 * PAGE_SIZE];

#define g_desc  ((vq_desc_t *)  g_queue_mem)
#define g_avail ((vq_avail_t *) (g_queue_mem + QUEUE_SIZE * 16u))
#define g_used  ((vq_used_t *)  (g_queue_mem + PAGE_SIZE))

static void setup_requestq(u32 base) {
    g_avail->flags = 0;
    g_avail->idx   = 0;
    g_used->flags  = 0;
    g_used->idx    = 0;

    VMIO(base, VMIO_GUEST_PAGE_SIZE) = PAGE_SIZE;
    VMIO(base, VMIO_QUEUE_SEL)       = 0;
    const u32 qmax                   = VMIO(base, VMIO_QUEUE_NUM_MAX);
    VMIO(base, VMIO_QUEUE_NUM)       = qmax < QUEUE_SIZE ? qmax : QUEUE_SIZE;
    VMIO(base, VMIO_QUEUE_ALIGN)     = PAGE_SIZE;
    VMIO(base, VMIO_QUEUE_PFN)       = virt_to_phys(g_queue_mem) >> 12;
}

// Build a 3-descriptor chain at slots 0/1/2 and poll for completion.
// Safe for synchronous single-threaded use only.
static bool do_request(u32 type, u64 sector, u32 count, const void *buf) {
    static virtio_blk_req_hdr_t hdr;
    static u8                   status;

    hdr    = (virtio_blk_req_hdr_t){.type = type, .sector = sector};
    status = 0xFF; // sentinel — device overwrites with result

    // Desc 0: request header (device reads)
    g_desc[0] = (vq_desc_t){
        .addr  = virt_to_phys(&hdr),
        .len   = sizeof(hdr),
        .flags = VDESC_F_NEXT,
        .next  = 1,
    };
    // Desc 1: data buffer (device writes for IN, reads for OUT)
    g_desc[1] = (vq_desc_t){
        .addr  = virt_to_phys(buf),
        .len   = count * 512u,
        .flags = (u16) ((type == VIRTIO_BLK_T_IN ? VDESC_F_WRITE : 0u) | VDESC_F_NEXT),
        .next  = 2,
    };
    // Desc 2: status byte (device always writes)
    g_desc[2] = (vq_desc_t){
        .addr  = virt_to_phys(&status),
        .len   = 1,
        .flags = VDESC_F_WRITE,
    };

    g_avail->ring[g_avail->idx % QUEUE_SIZE] = 0; // head = desc 0
    asm volatile("dsb" ::: "memory");
    g_avail->idx++;
    asm volatile("dsb" ::: "memory");
    VMIO(g_blk_base, VMIO_QUEUE_NOTIFY) = 0;

    while (g_used->idx == g_last_used_idx)
        asm volatile("" ::: "memory");
    g_last_used_idx++;

    return status == VIRTIO_BLK_S_OK;
}

bool blk_read(u64 sector, u32 count, void *buf) {
    return do_request(VIRTIO_BLK_T_IN, sector, count, buf);
}

bool blk_write(u64 sector, u32 count, const void *buf) {
    return do_request(VIRTIO_BLK_T_OUT, sector, count, buf);
}

void blk_init(void) {
    u32 base = virtio_find_device(VIRTIO_DEVICE_BLOCK, &blk_irq);
    if (!base) {
        warn("blk: no virtio-blk device found");
        return;
    }
    info("blk: found at base=0x%x irq=%d", base, blk_irq);
    g_blk_base = base;

    VMIO(base, VMIO_STATUS) = 0;
    VMIO(base, VMIO_STATUS) = VSTAT_ACKNOWLEDGE | VSTAT_DRIVER;

    VMIO(base, VMIO_GUEST_FEAT_SEL) = 0;
    VMIO(base, VMIO_GUEST_FEAT)     = 0;

    setup_requestq(base);
    VMIO(base, VMIO_STATUS) = VSTAT_ACKNOWLEDGE | VSTAT_DRIVER | VSTAT_DRIVER_OK;
    info("blk: ready, status=0x%x", VMIO(base, VMIO_STATUS));
    asm volatile("dsb" ::: "memory");
}