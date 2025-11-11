//
// Created by Romir Kulshrestha on 20/08/2025.
//

#include "main.h"

#include <logs.h>

#include "memory.h"
#include "utils.h"
#include "fdt/fdt.h"

int *(*x)(char);

int main() {
    const auto addr = (u32) FDT_ADDR;
    const auto page_index = (addr & 0b00000000000000000000111111111111);
    const auto l2___index = (addr & 0b00000000000011111111000000000000) >> 12;
    const auto l1___index = (addr & 0b11111111111100000000000000000000) >> 20;
    dbg("FDT Addr: 0x%x", addr);
    dbg("L1 Index: 0x%x", l1___index);
    dbg("L2 Index: 0x%x", l2___index);
    dbg("Page Index: 0x%x", page_index);

    // First level lookup
    const auto l1_entry = kernel_translation_table[l1___index];
    if (l1_entry.type != L1_PAGE_TABLE) {
        err("L1 entry is not a page table!");
        return -1;
    }

    const auto l2_table_addr = (l1_entry.page_table.address << 10) + VIRTUAL_OFFSET;
    dbg("L1 Entry Addr: 0x%p", (void *) l2_table_addr);

    // Second level lookup
    const auto l2_table = (l2_entry *) l2_table_addr;
    const auto l2_entry = l2_table[l2___index];
    if (l2_entry.type != L2_SMALL_PAGE) {
        err("L2 entry is not a small page!");
        return -1;
    }

    // Final physical address
    const auto phys_addr = (l2_entry.small_page.address << 12) | page_index;
    dbg("Physical Addr: 0x%x", phys_addr);

    return 0;
}
