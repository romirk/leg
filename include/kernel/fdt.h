// fdt.h — raw Flattened Device Tree binary format (all fields big-endian)

#ifndef FDT_H
#define FDT_H

#include "types.h"

#define FDT_MAGIC 0xd00dfeed

// FDT blob header — always at offset 0 in the DTB
struct fdt_header {
    u32 magic;             // must equal FDT_MAGIC
    u32 totalsize;         // total size of the blob in bytes
    u32 off_dt_struct;     // offset to structure block
    u32 off_dt_strings;    // offset to strings block
    u32 off_mem_rsvmap;    // offset to memory reservation block
    u32 version;           // format version (current = 17)
    u32 last_comp_version; // oldest backward-compatible version
    u32 boot_cpuid_phys;   // physical CPU id of boot CPU
    u32 size_dt_strings;   // size of strings block
    u32 size_dt_struct;    // size of structure block
};

// one reserved memory region entry; list is terminated by a zero entry
struct fdt_reserve_entry {
    u64 address;
    u64 size;
};

// token values used in the structure block
enum fdt_token {
    FDT_BEGIN_NODE = 0x1, // start of a node; followed by NUL-terminated name
    FDT_END_NODE = 0x2,   // end of a node
    FDT_PROP = 0x3,       // a property; followed by fdt_prop then data
    FDT_NOP = 0x4,        // padding/no-op
    FDT_END = 0x9,        // end of structure block
};

// property descriptor, immediately followed by `len` bytes of property data
struct fdt_prop {
    u32 len;      // length of the property data
    u32 name_off; // offset into strings block for the property name
};

#endif // FDT_H
