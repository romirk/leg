//
// Created by Romir Kulshrestha on 03/06/2025.
//

#include "fdt/parser.h"

#include "logs.h"
#include "stdlib.h"
#include "string.h"
#include "fdt/fdt.h"

[[gnu::const]]
static void *align(void *ptr, const u8 alignment) {
    return (void *) (-alignment & (u32) ptr + alignment - 1);
}

struct fdt_parse_result parse_fdt(struct fdt_header *header) {
    char *ptr = (char *) header + header->off_dt_struct;
    const char *strings = (char *) header + header->off_dt_strings;

    bool running = true;
    struct fdt_parse_result result = {.addr = 0};
    u8 addr_cells = 0, size_cells = 0;
    const char *curr_name = nullptr;

    dbg("parsing fdt");
    while (running) {
        const enum fdt_token token = swap_endianness(*(u32 *) ptr);
        ptr += sizeof(u32);

        switch (token) {
            case FDT_BEGIN_NODE: {
                curr_name = ptr;
                const auto len = strlen(ptr);
                dbg("node begin: %s", ptr);
                ptr += len + 1;
                ptr = align(ptr, 4);
                break;
            }
            case FDT_PROP: {
                auto prop = *(struct fdt_prop *) ptr;
                prop.name_off = swap_endianness(prop.name_off);
                prop.len = swap_endianness(prop.len);
                const auto prop_name = strings + prop.name_off;
                dbg("\tprop: %s", prop_name);

                ptr = ptr + sizeof(struct fdt_prop);
                if (!*curr_name) {
                    if (!strcmp(prop_name, "#address-cells")) {
                        addr_cells = swap_endianness(*(u32 *) ptr);
                    } else if (!strcmp(prop_name, "#size-cells")) {
                        size_cells = swap_endianness(*(u32 *) ptr);
                    }
                } else if (!strncmp(curr_name, "memory@", 7) && !strcmp(prop_name, "reg")) {
                    if (!addr_cells || !size_cells)
                        panic("Invalid FDT!");

                    const u32 *reg_ptr = (void *) ptr;
                    for (u8 i = 0; i < addr_cells; i++) {
                        result.addr = swap_endianness(*reg_ptr++);
                    }
                    for (u8 i = 0; i < size_cells; i++) {
                        result.size = swap_endianness(*reg_ptr++);
                    }
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
                err("unknown FDT token %x", token);
                running = false;
        }
    }

    dbg("parsed fdt");
    return result;
}
