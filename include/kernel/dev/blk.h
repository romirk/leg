#ifndef LEG_BLK_H
#define LEG_BLK_H

#include "types.h"

// GIC IRQ for the virtio-blk device; 0 if no device found during blk_init().
extern u32 blk_irq;

void blk_init(void);

// Synchronous sector I/O (512 bytes/sector).  Returns true on success.
bool blk_read(u64 sector, u32 count, void *buf);
bool blk_write(u64 sector, u32 count, const void *buf);

#endif // LEG_BLK_H