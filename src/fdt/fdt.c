//
// Created by Romir Kulshrestha on 02/06/2025.
//

#include "fdt/fdt.h"

#include "stdlib.h"
#include "uart.h"

void fdt_print_header(struct fdt_header *header) {
    char res[12];

    println("FDT Header information\n----------------------");

    print("Total size: ");
    itoa(header->totalsize, res);
    println(res);

    print("Version: ");
    itoa(header->version, res);
    println(res);

    print("Last compatible version: ");
    itoa(header->last_comp_version, res);
    println(res);

    print("Strings block bytes: ");
    itoa(header->size_dt_strings, res);
    println(res);

    print("Structure block bytes: ");
    itoa(header->size_dt_struct, res);
    println(res);
}
