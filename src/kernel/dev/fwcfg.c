// fw-cfg — QEMU firmware configuration interface (MMIO)

#include "kernel/dev/fwcfg.h"
#include "kernel/dev/memory.h"
#include "kernel/dev/mmu.h"
#include "kernel/logs.h"
#include "libc/bswap.h"
#include "libc/cstring.h"

static void fwcfg_select(u16 sel) {
    // QEMU applies DEVICE_BIG_ENDIAN (bswap on LE guests),
    // so we pre-swap so the device gets the correct selector.
    FWCFG_SEL = bswap16(sel);
}

static void fwcfg_read(void *buf, u32 len) {
    u8 *p = buf;
    for (u32 i = 0; i < len; i++)
        p[i] = FWCFG_DATA;
}

// control flags written to fwcfg_dma_access.control (big-endian to device)
typedef enum {
    FWCFG_DMA_ERROR = 1 << 0, // set by device on error
    FWCFG_DMA_READ = 1 << 1,
    FWCFG_DMA_SKIP = 1 << 2,
    FWCFG_DMA_SELECT = 1 << 3, // select a file by embedding selector in bits [31:16]
    FWCFG_DMA_WRITE = 1 << 4,
} fwcfg_dma_ctrl_t;

// DMA command descriptor written to FWCFG_DMA_LO; all fields big-endian
struct [[gnu::packed, gnu::aligned(4)]] fwcfg_dma_access {
    u32 control; // fwcfg_dma_ctrl_t flags | (selector << 16)
    u32 length;  // byte count to transfer
    u64 address; // physical address of the data buffer
};

static volatile struct fwcfg_dma_access dma_cmd;

void fwcfg_dma_write(u16 selector, const void *buf, u32 len) {
    u32 buf_phys = virt_to_phys(buf);

    dma_cmd.control = bswap32(((u32) selector << 16) | FWCFG_DMA_SELECT | FWCFG_DMA_WRITE);
    dma_cmd.length = bswap32(len);
    dma_cmd.address = bswap64(buf_phys);

    u32 dma_phys = virt_to_phys((void *) &dma_cmd);

    asm volatile("dsb" ::: "memory");
    FWCFG_DMA_HI = 0;
    FWCFG_DMA_LO = bswap32(dma_phys);

    while (dma_cmd.control != 0)
        asm volatile("dmb");
}

void fwcfg_init(void) {
    fwcfg_select(FWCFG_SIG_SEL);
    char sig[4];
    fwcfg_read(sig, 4);
    if (sig[0] != 'Q' || sig[1] != 'E' || sig[2] != 'M' || sig[3] != 'U')
        err("fw-cfg: bad signature");
    dbg("fw-cfg OK");
}

i32 fwcfg_find(const char *name) {
    fwcfg_select(FWCFG_DIR_SEL);
    u32 count_be;
    fwcfg_read(&count_be, 4);
    u32 count = bswap32(count_be);

    for (u32 i = 0; i < count; i++) {
        u32  size_be;
        u16  sel_be, reserved;
        char fname[56];
        fwcfg_read(&size_be, 4);
        fwcfg_read(&sel_be, 2);
        fwcfg_read(&reserved, 2);
        fwcfg_read(fname, 56);

        if (strcmp(fname, name) == 0) return bswap16(sel_be);
    }
    return -1;
}
