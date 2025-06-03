//
// Created by Romir Kulshrestha on 02/06/2025.
//

#include "fdt/fdt.h"

#include "stdio.h"
#include "stdlib.h"

void fdt_print_header(struct fdt_header *header) {
    char res[12];

    println("FDT Header information\n----------------------");

    print("Total size: ");
    utoa(header->totalsize, res, 10);
    println(res);

    print("Version: ");
    utoa(header->version, res, 10);
    println(res);

    print("Last compatible version: ");
    utoa(header->last_comp_version, res, 10);
    println(res);

    print("Strings block bytes: ");
    utoa(header->size_dt_strings, res, 10);
    println(res);

    print("Structure block bytes: ");
    utoa(header->size_dt_struct, res, 10);
    println(res);
}
