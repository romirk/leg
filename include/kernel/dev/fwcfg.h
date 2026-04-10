// fwcfg.h — QEMU firmware configuration interface (MMIO)

#ifndef LEG_FWCFG_H
#define LEG_FWCFG_H

#include "types.h"
#include "memory.h"

// Register offsets (ARM virt: data=+0, selector=+8, DMA=+0x10)
#define FWCFG_DATA   (*(volatile u8 *) (FWCFG_BASE + 0x00))
#define FWCFG_SEL    (*(volatile u16 *) (FWCFG_BASE + 0x08))
#define FWCFG_DMA_HI (*(volatile u32 *) (FWCFG_BASE + 0x10))
#define FWCFG_DMA_LO (*(volatile u32 *) (FWCFG_BASE + 0x14))

// Well-known selectors
#define FWCFG_SIG_SEL 0x0000u
#define FWCFG_DIR_SEL 0x0019u

void fwcfg_init(void);

// Find a file by name. Returns selector (>=0) or -1 if not found.
i32 fwcfg_find(const char *name);

// Write `len` bytes from `buf` to the fw-cfg file at `selector` via DMA.
// `buf` must be in identity-mapped or kernel-mapped memory.
void fwcfg_dma_write(u16 selector, const void *buf, u32 len);

#endif // LEG_FWCFG_H
