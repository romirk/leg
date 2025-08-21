//
// Created by Romir Kulshrestha on 20/08/2025.
//

#include "main.h"

#include <logs.h>

#include "memory.h"
#include "utils.h"

int main() {
    const auto addr = (void *) 0x40000000;
    const auto l1_idx = get_high_bits(addr, 12);
    const auto l2_idx = get_bits(addr, 19, 12);
    // const auto pg_idx = get_bits(addr, 11, 0);

    const auto l1 = kernel_translation_table[l1_idx];
    const auto l2_addr = (l2_table_entry *) (l1.page_table.address << 10);
    info("L1 lookup\n"
         "\ttype:\t%c\n"
         "\taddr:\t%p",
         "-PSX"[l1.type], l2_addr);

    const auto l2 = l2_addr[l2_idx];
    const auto pa = (void *) (l2.small_page.address << 12);
    info("L2 lookup\n"
        "\ttype:\t%c\n"
        "\taddr:\t%p",
        "-LSX"[l2.type], pa);
    return 0;
}
