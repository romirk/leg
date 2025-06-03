//
// Created by Romir Kulshrestha on 02/06/2025.
//

#include "fdt/fdt.h"

#include "stdio.h"
#include "stdlib.h"

void fdt_print_header(struct fdt_header *header) {
    printf("FDT Header information\n"
           "----------------------\n"
           "Total size: %u\n"
           "Version: %u\n"
           "Last compatible version: %u\n"
           ,
           header->totalsize,
           header->version,
           header->last_comp_version
    );
}
