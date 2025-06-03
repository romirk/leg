//
// Created by Romir Kulshrestha on 03/06/2025.
//

#include "fdt/parser.h"

#include "stdlib.h"
#include "string.h"
#include "uart.h"
#include "fdt/fdt.h"

[[gnu::const]]
static void *align(void *ptr, const u8 alignment) {
    return (void *) (-alignment & (u32) ptr + alignment - 1);
}

struct fdt_parse_result parse_fdt(struct fdt_header *header) {
    char *ptr = (char *) header + header->off_dt_struct;
    char *strings = (char *) header + header->off_dt_strings;

    bool running = true;
    struct fdt_parse_result result = {.reg_count = 0, .mem_regs = {0}};
    const char *curr_name = nullptr;

    while (running) {
        const enum fdt_token token = swap_endianness(*(u32 *) ptr);
        ptr += sizeof(u32);

        switch (token) {
            case FDT_BEGIN_NODE: {
                curr_name = ptr;
                const auto len = strlen(ptr);
                ptr += len + 1;
                ptr = align(ptr, 4);
                break;
            }
            case FDT_PROP: {
                auto prop = *(struct fdt_prop *) ptr;
                prop.name_off = swap_endianness(prop.name_off);
                prop.len = swap_endianness(prop.len);
                const auto prop_name = strings + prop.name_off;

                ptr = ptr + sizeof(struct fdt_prop);

                if (!*curr_name) {
                    if (!strcmp(prop_name, "#address-cells")) {
                        result.addr_cells = swap_endianness(*(u32 *) ptr);
                    }
                    if (!strcmp(prop_name, "#size-cells")) {
                        result.size_cells = swap_endianness(*(u32 *) ptr);
                    }
                } else if (!strncmp(curr_name, "memory@", 7) && !strcmp(prop_name, "reg")) {
                    if (!result.addr_cells || !result.size_cells)
                        panic("Invalid FDT!");

                    struct fdt_mem_reg reg;
                    const u32* reg_ptr = (void*)ptr;
                    for (u8 i = 0; i < result.addr_cells; i++) {
                        reg.addr[i] = swap_endianness(*reg_ptr++);
                    }
                    for (u8 i = 0; i < result.size_cells; i++) {
                        reg.size[i] = swap_endianness(*reg_ptr++);
                    }

                    result.mem_regs[result.reg_count++] = reg;
                }

                ptr = align(ptr + prop.len, 4);
                break;
            }
            case FDT_NOP:
            case FDT_END_NODE:
                break;
            case FDT_END:
                running = false;
                break;
            default:
                panic("unknown token");
        }
    }

    return result;
}
