#pragma once

#include "dev/mmu.h"
#include "types.h"

#define LLF_MAGIC   0x4C4C4646u // "LLFF"
#define LLF_VERSION 0x00010000u // version 1.0.0

typedef enum : u32 {
    LLF_SEG_NULL = 0,   // unused / invalid
    LLF_SEG_LOAD = 1,   // normal loadable segment (CODE/DATA)
    LLF_SEG_NOBITS = 2, // BSS (NOLOAD)

    LLF_SEG_MAX
} llf_segment_type_t;

typedef struct {
    u32 magic;
    u32 version;

    u32 entry;    // entry virtual address
    u32 ph_off;   // program header table offset
    u32 ph_count; // number of segments

    u32 flags; // reserved for future use, must be 0
} llf_header_t;

typedef struct {
    llf_segment_type_t type;
    u32                flags; // R/W/X

    u32 vaddr;  // VA mapping start
    u32 memsz;  // memory size (pages depend on this)
    u32 filesz; // file size (0 for BSS)

    u32 offset; // file offset
    u32 align;  // page alignment
} llf_phdr_t;

// Parse an LLF image from buf and map its segments into pgd.
// On success, sets *out_entry to the program entry VA and returns true.
bool llf_load(l1_entry *pgd, const void *buf, u32 buf_size, uptr *out_entry);