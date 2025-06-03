//
// Created by Romir Kulshrestha on 02/06/2025.
//

#include "fdt/fdt.h"

void fdt_endianness_swap(struct fdt_header *header) {
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