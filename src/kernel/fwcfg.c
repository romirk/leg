//
// fw-cfg — QEMU firmware configuration interface (MMIO)
//

#include "kernel/fwcfg.h"
#include "kernel/mmu.h"
#include "kernel/logs.h"
#include "libc/string.h"
#include "bswap.h"
#include "utils.h"

// --- low-level register access ---

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

// --- DMA ---

#define FWCFG_DMA_ERROR  0x01u
#define FWCFG_DMA_READ   0x02u
#define FWCFG_DMA_SKIP   0x04u
#define FWCFG_DMA_SELECT 0x08u
#define FWCFG_DMA_WRITE  0x10u

struct [[gnu::packed, gnu::aligned(4)]] fwcfg_dma_access {
    u32 control;
    u32 length;
    u64 address;
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

// --- init + directory ---

void fwcfg_init(void) {
    fwcfg_select(FWCFG_SIG_SEL);
    char sig[4];
    fwcfg_read(sig, 4);
    if (sig[0] != 'Q' || sig[1] != 'E' || sig[2] != 'M' || sig[3] != 'U')
        panic("fw-cfg: bad signature");
    dbg("fw-cfg OK");
}

i32 fwcfg_find(const char *name) {
    fwcfg_select(FWCFG_DIR_SEL);
    u32 count_be;
    fwcfg_read(&count_be, 4);
    u32 count = bswap32(count_be);

    for (u32 i = 0; i < count; i++) {
        u32 size_be;
        u16 sel_be, reserved;
        char fname[56];
        fwcfg_read(&size_be, 4);
        fwcfg_read(&sel_be, 2);
        fwcfg_read(&reserved, 2);
        fwcfg_read(fname, 56);

        if (strcmp(fname, name) == 0)
            return bswap16(sel_be);
    }
    return -1;
}
