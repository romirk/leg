#include "kernel/dev/kbd.h"

#include "kernel/dev/memory.h"
#include "kernel/dev/virtio.h"
#include "kernel/logs.h"
#include "kernel/mem/alloc.h"
#include "kernel/tty.h"

#define EV_KEY 1u
#define KEY_Q  16u

#define KEY_LEFTSHIFT  42u
#define KEY_RIGHTSHIFT 54u

u32         kbd_irq;
static bool g_shift;

// Linux keycode → ASCII, unshifted and shifted.
// Index = Linux keycode (0–57 covers all basic keys).
constexpr char keymap_lower[58] = {
    0,   0,                      // 0 reserved, 1 ESC
    '1', '2',  '3',  '4',  '5',  // 2-6
    '6', '7',  '8',  '9',  '0',  // 7-11
    '-', '=',  '\b', '\t',       // 12-15
    'q', 'w',  'e',  'r',  't',  // 16-20
    'y', 'u',  'i',  'o',  'p',  // 21-25
    '[', ']',  '\n', 0,          // 26-29 (29 = LEFTCTRL)
    'a', 's',  'd',  'f',  'g',  // 30-34
    'h', 'j',  'k',  'l',        // 35-38
    ';', '\'', '`',  0,    '\\', // 39-43 (42 = LEFTSHIFT)
    'z', 'x',  'c',  'v',  'b',  // 44-48
    'n', 'm',  ',',  '.',  '/',  // 49-53
    0,   '*',  0,    ' ',        // 54-57 (54=RIGHTSHIFT, 56=LEFTALT)
};

constexpr char keymap_upper[58] = {
    0,    0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|',  'Z',
    'X',  'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ',
};

void handle_kbd_event(const virtio_input_event_t *ev) {
    if (ev->type == EV_KEY) {
        if (ev->code == KEY_LEFTSHIFT || ev->code == KEY_RIGHTSHIFT)
            g_shift = ev->value != 0;
        else if ((ev->value == 1 || ev->value == 2) && ev->code < 58u) {
            const char c = g_shift ? keymap_upper[ev->code] : keymap_lower[ev->code];
            if (c) tty_input(c);
        }
    }
}

void kbd_handle_char(const char c) {
    tty_input(c);
}

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

static void setup_eventq(u32 base) {
    for (u32 i = 0; i < QUEUE_SIZE; i++) {
        g_desc[i] = (vq_desc_t) {
            .addr  = virt_to_phys(&g_events[i]),
            .len   = sizeof(virtio_input_event_t),
            .flags = VDESC_F_WRITE,
        };
        g_avail->ring[i] = (u16) i;
    }
    g_avail->flags = 0;
    g_avail->idx   = QUEUE_SIZE;
    g_used->flags  = 0;
    g_used->idx    = 0;

    VMIO(base, VMIO_GUEST_PAGE_SIZE) = PAGE_SIZE;
    VMIO(base, VMIO_QUEUE_SEL)       = 0;
    const u32 qmax                   = VMIO(base, VMIO_QUEUE_NUM_MAX);
    VMIO(base, VMIO_QUEUE_NUM)       = qmax < QUEUE_SIZE ? qmax : QUEUE_SIZE;
    VMIO(base, VMIO_QUEUE_ALIGN)     = PAGE_SIZE;
    VMIO(base, VMIO_QUEUE_PFN)       = virt_to_phys(g_queue_mem) >> 12;
}

void kbd_irq_handler(void) {
    u32 isr                           = VMIO(g_virtio_base, VMIO_INT_STATUS);
    VMIO(g_virtio_base, VMIO_INT_ACK) = isr;
    asm volatile("dsb" ::: "memory");

    while (g_last_used_idx != g_used->idx) {
        u32                   desc_id = g_used->ring[g_last_used_idx % QUEUE_SIZE].id;
        virtio_input_event_t *ev      = &g_events[desc_id];

        handle_kbd_event(ev);

        g_avail->ring[g_avail->idx % QUEUE_SIZE] = (u16) desc_id;
        g_avail->idx++;
        g_last_used_idx++;
    }

    asm volatile("dsb" ::: "memory");
    VMIO(g_virtio_base, VMIO_QUEUE_NOTIFY) = 0;
}

void kbd_init(void) {
    u32 base = virtio_find_device(VIRTIO_DEVICE_INPUT, &kbd_irq);
    if (!base) {
        warn("kbd: no virtio-input device found");
        return;
    }
    info("kbd: found at base=0x%x irq=%d", base, kbd_irq);
    g_virtio_base = base;

    VMIO(base, VMIO_STATUS) = 0;
    VMIO(base, VMIO_STATUS) = VSTAT_ACKNOWLEDGE | VSTAT_DRIVER;

    VMIO(base, VMIO_GUEST_FEAT_SEL) = 0;
    VMIO(base, VMIO_GUEST_FEAT)     = 0;

    setup_eventq(base);
    VMIO(base, VMIO_STATUS) = VSTAT_ACKNOWLEDGE | VSTAT_DRIVER | VSTAT_DRIVER_OK;
    info("kbd: ready, status=0x%x", VMIO(base, VMIO_STATUS));

    asm volatile("dsb" ::: "memory");
    VMIO(base, VMIO_QUEUE_NOTIFY) = 0;
}