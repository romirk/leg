//
// Created by Romir Kulshrestha on 03/06/2025.
//

#ifndef FDT_PARSER_H
#define FDT_PARSER_H

#include "fdt/fdt.h"

#define FDT_MAX_MEM_REGS 32

struct fdt_parse_result {
    u32 addr;
    u32 size;
    bool success;
};

struct fdt_parse_result parse_fdt(void);

#endif //FDT_PARSER_H