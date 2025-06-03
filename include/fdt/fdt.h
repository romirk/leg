//
// Created by Romir Kulshrestha on 02/06/2025.
//

#ifndef FDT_H
#define FDT_H

#include "stdlib.h"
#include "types.h"

#define FDT_ADDR    (void *)    0x40000000
#define FDT_MAGIC               0xd00dfeed

struct fdt_header {
    u32 magic;
    u32 totalsize;
    u32 off_dt_struct;
    u32 off_dt_strings;
    u32 off_mem_rsvmap;
    u32 version;
    u32 last_comp_version;
    u32 boot_cpuid_phys;
    u32 size_dt_strings;
    u32 size_dt_struct;
};

struct fdt_reserve_entry {
    u64 address;
    u64 size;
};

enum fdt_token {
    FDT_BEGIN_NODE = 0x1,
    FDT_END_NODE = 0x2,
    FDT_PROP = 0x3,
    FDT_NOP = 0x4,
    FDT_END = 0x9,
};

struct fdt_prop {
    u32 len;
    u32 name_off;
};

void fdt_endianness_swap(struct fdt_header *header);

#endif //FDT_H
