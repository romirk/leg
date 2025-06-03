//
// Created by Romir Kulshrestha on 03/06/2025.
//

#ifndef FDT_PARSER_H
#define FDT_PARSER_H

#include "fdt/fdt.h"

#define FDT_MAX_MEM_REGS 32

struct fdt_mem_reg {
    u32 addr[2];
    u32 size[2];
};

struct fdt_parse_result {
    u8 addr_cells;
    u8 size_cells;
    u8 reg_count;
    struct fdt_mem_reg mem_regs[FDT_MAX_MEM_REGS];
};

struct fdt_parse_result parse_fdt(struct fdt_header *header);

#endif //FDT_PARSER_H
