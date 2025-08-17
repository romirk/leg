//
// Created by Romir Kulshrestha on 03/06/2025.
//

#include "fdt/parser.h"

#include "logs.h"
#include "stdlib.h"
#include "string.h"
#include "utils.h"
#include "fdt/fdt.h"

static void fdt_endianness_swap(struct fdt_header *header) {
    header->magic = swap_endianness(header->magic);
    header->totalsize = swap_endianness(header->totalsize);
    header->off_dt_struct = swap_endianness(header->off_dt_struct);
    header->off_dt_strings = swap_endianness(header->off_dt_strings);
    header->off_mem_rsvmap = swap_endianness(header->off_mem_rsvmap);
    header->version = swap_endianness(header->version);
    header->last_comp_version = swap_endianness(header->last_comp_version);
    header->boot_cpuid_phys = swap_endianness(header->boot_cpuid_phys);
    header->size_dt_strings = swap_endianness(header->size_dt_strings);
    header->size_dt_struct = swap_endianness(header->size_dt_struct);
}

static const char* indent(u8 depth) {
    static const char indent_buffer[] = "\t\t\t\t\t\t\t\t\t\t";
    return indent_buffer + (10 - depth);
}

struct fdt_parse_result parse_fdt() {
    auto header = (struct fdt_header *) FDT_ADDR;
    fdt_endianness_swap(header);

    struct fdt_parse_result result = {.success = false};

    if (header->magic != FDT_MAGIC) {
        puts("Not a valid FDT header!");
        return result;
    }

    char *ptr = (char *) header + header->off_dt_struct;
    const char *strings = (char *) header + header->off_dt_strings;

    u8 addr_cells = 0, size_cells = 0, depth = 0;
    const char *curr_name = nullptr;

    dbg("parsing fdt");

    while (true) {
        const enum fdt_token token = swap_endianness(*(u32 *) ptr);
        ptr += sizeof(u32);

        switch (token) {
            case FDT_BEGIN_NODE: {
                curr_name = ptr;
                const auto len = strlen(ptr);
                dbg("%s= %s", indent(depth), ptr);
                ptr += len + 1;
                ptr = align(ptr, 4);
                depth++;
                break;
            }
            case FDT_PROP: {
                auto prop = *(struct fdt_prop *) ptr;
                prop.name_off = swap_endianness(prop.name_off);
                prop.len = swap_endianness(prop.len);
                const auto prop_name = strings + prop.name_off;
                dbg("%s- %s", indent(depth), prop_name);

                ptr = ptr + sizeof(struct fdt_prop);
                if (!*curr_name) {
                    if (!strcmp(prop_name, "#address-cells")) {
                        addr_cells = swap_endianness(*(u32 *) ptr);
                    } else if (!strcmp(prop_name, "#size-cells")) {
                        size_cells = swap_endianness(*(u32 *) ptr);
                    }
                } else if (!addr_cells || !size_cells)
                    return result;
                else if (!strncmp(curr_name, "memory@", 7) && !strcmp(prop_name, "reg")) {
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
            case FDT_END_NODE:
                depth--;
            case FDT_NOP:
                break;
            case FDT_END:
                result.success = true;
                return result;
            default:
                err("unknown FDT token %x", token);
                return result;
        }
    }
}