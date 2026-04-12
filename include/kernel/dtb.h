#pragma once

// DTB (Device Tree Blob / Flattened Device Tree) Parser
// Target: ARMv7a bare-metal kernel, C23, clang/GCC extensions
// Spec: https://devicetree-specification.readthedocs.io/en/stable/

#include "types.h"

// Allocator interface
typedef void *(*dtb_alloc_fn)(u32 size);

// Limits
#ifndef DTB_MAX_NODES
#define DTB_MAX_NODES 256
#endif
#ifndef DTB_MAX_PROPS
#define DTB_MAX_PROPS 1024
#endif

// DTB blob header — always at offset 0 in the DTB
struct dtb_header {
    u32 magic;             // must equal DTB_MAGIC
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
struct dtb_reserve_entry {
    u64 address;
    u64 size;
};

// token values used in the structure block
enum dtb_token {
    DTB_BEGIN_NODE = 0x1, // start of a node; followed by NUL-terminated name
    DTB_END_NODE = 0x2,   // end of a node
    DTB_PROP = 0x3,       // a property; followed by DTB_prop then data
    DTB_NOP = 0x4,        // padding/no-op
    DTB_END = 0x9,        // end of structure block
};

// Property value types (heuristically decoded or explicit)
typedef enum : u8 {
    DTB_VAL_EMPTY = 0,   // no data
    DTB_VAL_U32 = 1,     // single big-endian u32
    DTB_VAL_U64 = 2,     // two big-endian u32s → u64
    DTB_VAL_STRING = 3,  // null-terminated string
    DTB_VAL_STRINGS = 4, // stringlist (multiple NUL-terminated)
    DTB_VAL_BYTES = 5,   // raw byte array
    DTB_VAL_CELLS = 6,   // array of big-endian u32 cells
} dtb_val_type_t;

// A parsed property
typedef struct dtb_prop {
    const char    *name; // pointer into string block
    const u8      *data; // pointer into raw DTB blob
    u32            len;  // byte length of data
    dtb_val_type_t type; // decoded type hint
} dtb_prop_t;

// A parsed node
typedef struct dtb_node dtb_node_t;

struct dtb_node {
    const char *name; // node name (may include @unit-addr)
    dtb_node_t *parent;
    dtb_node_t *children; // linked list
    dtb_node_t *next_sibling;
    dtb_prop_t *props; // array
    u32         prop_count;
    u32         phandle; // 0 = none
    // #address-cells / #size-cells inherited or local
    u32 address_cells;
    u32 size_cells;
};

// Top-level parsed tree
typedef struct {
    dtb_node_t *root;

    // Convenience: directly resolved fields
    struct {
        u64 base;
        u64 size;
    } memory[8];

    u32         memory_count;
    u32         boot_cpuid;
    const char *compatible; // root compatible string (first entry)
} dtb_tree_t;

// Error codes
typedef enum : int {
    DTB_OK = 0,
    DTB_ERR_NULL = -1,      // null pointer
    DTB_ERR_MAGIC = -2,     // bad DTB magic
    DTB_ERR_VERSION = -3,   // unsupported DTB version
    DTB_ERR_TRUNCATED = -4, // blob smaller than reported
    DTB_ERR_NOMEM = -5,     // allocator returned NULL
    DTB_ERR_OVERFLOW = -6,  // exceeded DTB_MAX_* limit
    DTB_ERR_CORRUPT = -7,   // structural inconsistency
} dtb_err_t;

// API

// Parse a raw DTB blob.  `blob` must remain valid for the lifetime of `tree`.
// All allocations go through `alloc` (e.g. early_malloc).
dtb_err_t dtb_parse(const void *blob, u32 blob_len, dtb_tree_t *tree, dtb_alloc_fn alloc);

// Query helpers

// Find a node by path, e.g. "/cpus/cpu@0"
dtb_node_t *dtb_find_node(const dtb_tree_t *tree, const char *path);

// Find a node by phandle
dtb_node_t *dtb_find_phandle(const dtb_tree_t *tree, u32 phandle);

// Find first node whose compatible list contains `compat`
dtb_node_t *dtb_find_compatible(const dtb_tree_t *tree, const char *compat);

// Get a property from a node (NULL if not found)
const dtb_prop_t *dtb_get_prop(const dtb_node_t *node, const char *name);

// Typed value extractors (return false on type/size mismatch)
bool dtb_prop_u32(const dtb_prop_t *prop, u32 *out);

bool dtb_prop_u64(const dtb_prop_t *prop, u64 *out);

bool dtb_prop_string(const dtb_prop_t *prop, const char **out);

// Read a reg entry: (base, size) pair from a node's "reg" property
// `index` selects which pair.  Returns false if out of range.
bool dtb_reg_entry(const dtb_node_t *node, u32 index, u64 *base, u64 *size);

// Iterate children: returns first child on NULL, next sibling otherwise
dtb_node_t *dtb_child_first(const dtb_node_t *node);

dtb_node_t *dtb_child_next(const dtb_node_t *child);

// Print the entire parsed tree to UART (via printf)
void dtb_dump(const dtb_tree_t *tree);
