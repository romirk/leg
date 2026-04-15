// DTB (Flattened Device Tree) Parser
// C23, clang/GCC extensions, no libc dependency

#include "kernel/dtb.h"

#include "libc/bswap.h"
#include "libc/builtins.h"
#include "libc/cstring.h"
#include "libc/stdio.h"

#define DTB_MAGIC 0xd00dfeed

// internal state threaded through the recursive descent parser
typedef struct {
    const u8    *blob; // raw DTB blob
    u32          blob_len;
    const u8    *struct_block; // pointer into blob at off_dt_struct
    const char  *string_block; // pointer into blob at off_dt_strings
    u32          struct_size;
    u32          string_size;
    dtb_alloc_fn alloc;     // allocator for nodes and properties
    dtb_node_t  *node_pool; // pre-allocated node array
    u32          node_used;
    dtb_prop_t  *prop_pool; // pre-allocated property array
    u32          prop_used;
} parser_t;

static dtb_node_t *alloc_node(parser_t *p) {
    if (p->node_used >= DTB_MAX_NODES) return nullptr;
    dtb_node_t *n = &p->node_pool[p->node_used++];
    memset(n, 0, sizeof(*n));
    n->address_cells = 2;
    n->size_cells = 1;
    return n;
}

static dtb_prop_t *alloc_props(parser_t *p, u32 count) {
    if (p->prop_used + count > DTB_MAX_PROPS) return nullptr;
    dtb_prop_t *arr = &p->prop_pool[p->prop_used];
    p->prop_used += count;
    return arr;
}

// Bounds-checked access
static bool in_struct(const parser_t *p, const u8 *ptr, u32 len) {
    const u32 base = (u32) p->struct_block;
    const u32 end = base + p->struct_size;
    const u32 addr = (u32) ptr;
    return addr >= base && addr + len <= end && addr + len >= addr;
}

// Type heuristic
static dtb_val_type_t guess_type(const char *name, const u8 *data, u32 len) {
    if (len == 0) return DTB_VAL_EMPTY;

    static const char *const u32_names[] = {
        "#address-cells", "#size-cells",     "#interrupt-cells", "interrupt-parent",
        "phandle",        "linux,phandle",   "clock-frequency",  "timebase-frequency",
        "virtual-reg",    "cache-line-size", "cache-sets",       "cache-size",
        nullptr};
    for (u32 i = 0; u32_names[i]; ++i)
        if (strcmp(name, u32_names[i]) == 0 && len == 4) return DTB_VAL_U32;

    if (strcmp(name, "clock-output-names") == 0 && len == 8) return DTB_VAL_U64;

    // Detect string / stringlist
    bool all_str = true;
    for (u32 i = 0; i < len; ++i) {
        const u8 c = data[i];
        if (c != 0 && (c < 0x20 || c > 0x7e)) {
            all_str = false;
            break;
        }
    }
    if (all_str && data[len - 1] == 0) {
        u32 nulls = 0;
        for (u32 i = 0; i < len; ++i)
            if (data[i] == 0) ++nulls;
        return nulls > 1 ? DTB_VAL_STRINGS : DTB_VAL_STRING;
    }

    if (len % 4 == 0) return DTB_VAL_CELLS;
    return DTB_VAL_BYTES;
}

// Recursive descent parser
static const u8 *parse_node(parser_t *p, const u8 *cur, dtb_node_t *parent, dtb_node_t **out_node) {
    // cur points past DTB_BEGIN_NODE token — read NUL-terminated name
    const auto name = (const char *) cur;
    const u32  name_offset = (u32) cur - (u32) p->struct_block;
    u32        name_len = 0;
    while (in_struct(p, cur + name_len, 1) && cur[name_len] != 0)
        ++name_len;
    const u32 padded = (name_offset + name_len + 1 + 3) & ~(u32) 3;
    cur = p->struct_block + padded;

    dtb_node_t *node = alloc_node(p);
    if (!node) return nullptr;

    node->name = name;
    node->parent = parent;
    if (parent) {
        node->address_cells = parent->address_cells;
        node->size_cells = parent->size_cells;
    }

    // Count props (look-ahead) to batch-allocate
    u32 prop_count = 0;
    {
        const u8 *scan = cur;
        u32       depth = 0;
        while (true) {
            if (!in_struct(p, scan, 4)) break;
            const u32 tok = be32_read(scan);
            scan += 4;
            if (tok == DTB_NOP) continue;
            if (tok == DTB_END_NODE) {
                if (depth == 0) break;
                --depth;
                continue;
            }
            if (tok == DTB_BEGIN_NODE) {
                ++depth;
                while (in_struct(p, scan, 1) && *scan != 0)
                    ++scan;
                ++scan;
                const u32 off = (u32) scan - (u32) p->struct_block;
                scan = p->struct_block + ((off + 3) & ~(u32) 3);
                continue;
            }
            if (tok == DTB_PROP) {
                if (!in_struct(p, scan, 8)) break;
                u32 plen = be32_read(scan);
                scan += 8;
                u32 off = (u32) scan - (u32) p->struct_block;
                scan = p->struct_block + ((off + plen + 3) & ~(u32) 3);
                if (depth == 0) ++prop_count;
                continue;
            }
            if (tok == DTB_END) break;
        }
    }

    dtb_prop_t *props = nullptr;
    if (prop_count > 0) {
        props = alloc_props(p, prop_count);
        if (!props) return nullptr;
    }
    node->props = props;
    node->prop_count = 0;

    // Main parse loop
    while (true) {
        if (!in_struct(p, cur, 4)) return nullptr;
        const u32 tok = be32_read(cur);
        cur += 4;

        switch (tok) {
            case DTB_NOP:
                continue;
            case DTB_END_NODE:
            case DTB_END:
                goto done;
            case DTB_PROP: {
                if (!in_struct(p, cur, 8)) return nullptr;
                const u32 plen = be32_read(cur);
                const u32 nameoff = be32_read(cur + 4);
                cur += 8;

                if (nameoff >= p->string_size) return nullptr;
                const char *pname = p->string_block + nameoff;
                const u8   *pdata = cur;
                u32         off = (u32) cur - (u32) p->struct_block;
                cur = p->struct_block + ((off + plen + 3) & ~(u32) 3);

                if (node->prop_count >= prop_count) return nullptr;
                dtb_prop_t *pr = &props[node->prop_count++];
                pr->name = pname;
                pr->data = pdata;
                pr->len = plen;
                pr->type = guess_type(pname, pdata, plen);

                if (strcmp(pname, "#address-cells") == 0 && plen == 4)
                    node->address_cells = be32_read(pdata);
                else if (strcmp(pname, "#size-cells") == 0 && plen == 4)
                    node->size_cells = be32_read(pdata);
                else if ((strcmp(pname, "phandle") == 0 || strcmp(pname, "linux,phandle") == 0) &&
                         plen == 4)
                    node->phandle = be32_read(pdata);
                break;
            }
            case DTB_BEGIN_NODE: {
                dtb_node_t *child = nullptr;
                cur = parse_node(p, cur, node, &child);
                if (!cur) return nullptr;
                if (!node->children) {
                    node->children = child;
                } else {
                    dtb_node_t *sib = node->children;
                    while (sib->next_sibling)
                        sib = sib->next_sibling;
                    sib->next_sibling = child;
                }
                break;
            }
            default:
                break;
        }
    }
done:
    *out_node = node;
    return cur;
}

// Memory node extraction
static void extract_memory(dtb_tree_t *tree, dtb_node_t *root) {
    for (dtb_node_t *n = root->children; n; n = n->next_sibling) {
        if (strncmp(n->name, "memory", 6) != 0) continue;
        const dtb_prop_t *dt = dtb_get_prop(n, "device_type");
        if (!dt) dt = dtb_get_prop(n, "device-type");
        if (dt && (dt->len == 0 || strcmp((const char *) dt->data, "memory") != 0)) continue;
        const dtb_prop_t *reg = dtb_get_prop(n, "reg");
        if (!reg) continue;
        u32 ac = root->address_cells;
        u32 sc = root->size_cells;
        u32 entry_bytes = (ac + sc) * 4;
        u32 entries = reg->len / entry_bytes;
        for (u32 i = 0; i < entries && tree->memory_count < 8; ++i) {
            const u8 *p = reg->data + i * entry_bytes;
            u64       base = 0, size = 0;
            for (u32 c = 0; c < ac; ++c)
                base = (base << 32) | be32_read(p + c * 4);
            for (u32 c = 0; c < sc; ++c)
                size = (size << 32) | be32_read(p + (ac + c) * 4);
            tree->memory[tree->memory_count].base = base;
            tree->memory[tree->memory_count].size = size;
            ++tree->memory_count;
        }
    }
}

// Public API: dtb_parse
dtb_err_t dtb_parse(const void *blob, u32 blob_len, dtb_tree_t *tree, dtb_alloc_fn alloc) {
    if (!blob || !tree || !alloc) return DTB_ERR_NULL;

    memset(tree, 0, sizeof(*tree));

    const u8 *raw = blob;

    if (blob_len < sizeof(struct dtb_header)) return DTB_ERR_TRUNCATED;

    if (be32_read(raw) != DTB_MAGIC) return DTB_ERR_MAGIC;

    u32 totalsize = be32_read(raw + 4);
    u32 off_struct = be32_read(raw + 8);
    u32 off_strings = be32_read(raw + 12);
    u32 boot_cpuid = be32_read(raw + 28);
    u32 size_strings = be32_read(raw + 32);
    u32 size_struct = be32_read(raw + 36);

    if (totalsize > blob_len) return DTB_ERR_TRUNCATED;
    if (off_struct + size_struct > totalsize) return DTB_ERR_TRUNCATED;
    if (off_strings + size_strings > totalsize) return DTB_ERR_TRUNCATED;

    tree->boot_cpuid = boot_cpuid;

    dtb_node_t *node_pool = alloc(DTB_MAX_NODES * sizeof(dtb_node_t));
    dtb_prop_t *prop_pool = alloc(DTB_MAX_PROPS * sizeof(dtb_prop_t));
    if (!node_pool || !prop_pool) return DTB_ERR_NOMEM;

    parser_t p = {
        .blob = raw,
        .blob_len = blob_len,
        .struct_block = raw + off_struct,
        .string_block = (const char *) (raw + off_strings),
        .struct_size = size_struct,
        .string_size = size_strings,
        .alloc = alloc,
        .node_pool = node_pool,
        .node_used = 0,
        .prop_pool = prop_pool,
        .prop_used = 0,
    };

    // Find first BEGIN_NODE
    const u8 *cur = p.struct_block;
    while (true) {
        if (!in_struct(&p, cur, 4)) return DTB_ERR_CORRUPT;
        u32 tok = be32_read(cur);
        if (tok == DTB_NOP) {
            cur += 4;
            continue;
        }
        if (tok == DTB_BEGIN_NODE) {
            cur += 4;
            break;
        }
        return DTB_ERR_CORRUPT;
    }

    dtb_node_t *root = nullptr;
    cur = parse_node(&p, cur, nullptr, &root);
    if (!cur || !root) return DTB_ERR_CORRUPT;

    tree->root = root;

    const dtb_prop_t *compat = dtb_get_prop(root, "compatible");
    if (compat && compat->len > 0) tree->compatible = (const char *) compat->data;

    extract_memory(tree, root);

    return DTB_OK;
}

// dtb_find_node — path must start with '/'
static dtb_node_t *find_in_children(dtb_node_t *node, const char *segment, u32 seg_len) {
    for (dtb_node_t *c = node->children; c; c = c->next_sibling) {
        u32 name_len = strlen(c->name);
        if (name_len == seg_len && strncmp(c->name, segment, seg_len) == 0) return c;
        // Match "foo" against "foo@addr"
        for (u32 i = 0; i < name_len; ++i) {
            if (c->name[i] == '@') {
                if (i == seg_len && strncmp(c->name, segment, seg_len) == 0) return c;
                break;
            }
        }
    }
    return nullptr;
}

dtb_node_t *dtb_find_node(const dtb_tree_t *tree, const char *path) {
    if (!tree || !path || path[0] != '/') return nullptr;
    dtb_node_t *cur = tree->root;
    ++path;
    while (*path && cur) {
        const char *sep = path;
        while (*sep && *sep != '/')
            ++sep;
        u32 seg_len = (u32) (sep - path);
        if (seg_len == 0) {
            path = sep + 1;
            continue;
        }
        cur = find_in_children(cur, path, seg_len);
        path = *sep ? sep + 1 : sep;
    }
    return cur;
}

// dtb_find_phandle — linear scan
static dtb_node_t *phandle_scan(dtb_node_t *node, u32 ph) {
    if (!node) return nullptr;
    if (node->phandle == ph) return node;
    for (dtb_node_t *c = node->children; c; c = c->next_sibling) {
        dtb_node_t *r = phandle_scan(c, ph);
        if (r) return r;
    }
    return nullptr;
}

dtb_node_t *dtb_find_phandle(const dtb_tree_t *tree, u32 phandle) {
    if (!tree || phandle == 0) return nullptr;
    return phandle_scan(tree->root, phandle);
}

// dtb_find_compatible
static bool node_has_compat(const dtb_node_t *node, const char *compat) {
    const dtb_prop_t *p = dtb_get_prop(node, "compatible");
    if (!p || p->len == 0) return false;
    auto        cur = (const char *) p->data;
    const char *end = cur + p->len;
    while (cur < end) {
        if (strcmp(cur, compat) == 0) return true;
        cur += strlen(cur) + 1;
    }
    return false;
}

static dtb_node_t *compat_scan(dtb_node_t *node, const char *compat) {
    if (!node) return nullptr;
    if (node_has_compat(node, compat)) return node;
    for (dtb_node_t *c = node->children; c; c = c->next_sibling) {
        dtb_node_t *r = compat_scan(c, compat);
        if (r) return r;
    }
    return nullptr;
}

dtb_node_t *dtb_find_compatible(const dtb_tree_t *tree, const char *compat) {
    if (!tree || !compat) return nullptr;
    return compat_scan(tree->root, compat);
}

// dtb_get_prop
const dtb_prop_t *dtb_get_prop(const dtb_node_t *node, const char *name) {
    if (!node || !name) return nullptr;
    for (u32 i = 0; i < node->prop_count; ++i)
        if (strcmp(node->props[i].name, name) == 0) return &node->props[i];
    return nullptr;
}

// Typed extractors
bool dtb_prop_u32(const dtb_prop_t *prop, u32 *out) {
    if (!prop || !out || prop->len < 4) return false;
    *out = be32_read(prop->data);
    return true;
}

bool dtb_prop_u64(const dtb_prop_t *prop, u64 *out) {
    if (!prop || !out) return false;
    if (prop->len == 4) {
        *out = be32_read(prop->data);
        return true;
    }
    if (prop->len == 8) {
        *out = be64_read(prop->data);
        return true;
    }
    return false;
}

bool dtb_prop_string(const dtb_prop_t *prop, const char **out) {
    if (!prop || !out || prop->len == 0) return false;
    *out = (const char *) prop->data;
    return true;
}

// dtb_reg_entry
bool dtb_reg_entry(const dtb_node_t *node, u32 index, u64 *base, u64 *size) {
    if (!node || !base || !size) return false;
    const dtb_prop_t *reg = dtb_get_prop(node, "reg");
    if (!reg) return false;

    u32 ac = node->parent ? node->parent->address_cells : 2;
    u32 sc = node->parent ? node->parent->size_cells : 1;
    u32 entry_bytes = (ac + sc) * 4;
    if (entry_bytes == 0 || (index + 1) * entry_bytes > reg->len) return false;

    const u8 *p = reg->data + index * entry_bytes;
    u64       b = 0, s = 0;
    for (u32 c = 0; c < ac; ++c)
        b = (b << 32) | be32_read(p + c * 4);
    for (u32 c = 0; c < sc; ++c)
        s = (s << 32) | be32_read(p + (ac + c) * 4);
    *base = b;
    *size = s;
    return true;
}

// Child iteration
dtb_node_t *dtb_child_first(const dtb_node_t *node) {
    return node ? node->children : nullptr;
}

dtb_node_t *dtb_child_next(const dtb_node_t *child) {
    return child ? child->next_sibling : nullptr;
}

// Tree dump
static void print_indent(u32 depth) {
    for (u32 i = 0; i < depth; ++i)
        printf("  ");
}

static void dump_prop(const dtb_prop_t *p, u32 depth) {
    print_indent(depth);
    printf("%s", p->name);
    if (p->len == 0) {
        printf(";\n");
        return;
    }
    printf(" = ");
    switch (p->type) {
        case DTB_VAL_STRING:
            printf("\"%s\"", (const char *) p->data);
            break;
        case DTB_VAL_STRINGS: {
            auto        s = (const char *) p->data;
            const char *end = s + p->len;
            bool        first = true;
            while (s < end) {
                if (!first) printf(", ");
                printf("\"%s\"", s);
                s += strlen(s) + 1;
                first = false;
            }
            break;
        }
        case DTB_VAL_U32:
            printf("<0x%p>", be32_read(p->data));
            break;
        case DTB_VAL_U64:
            printf("<0x%p 0x%p>", (u32) (be64_read(p->data) >> 32), (u32) be64_read(p->data));
            break;
        case DTB_VAL_CELLS: {
            printf("<");
            for (u32 i = 0; i < p->len / 4; ++i) {
                if (i) printf(" ");
                printf("0x%p", be32_read(p->data + i * 4));
            }
            printf(">");
            break;
        }
        default: {
            printf("[");
            for (u32 i = 0; i < p->len; ++i) {
                if (i) printf(" ");
                printf("%x", p->data[i]);
            }
            printf("]");
            break;
        }
    }
    printf(";\n");
}

static void dump_node(const dtb_node_t *node, u32 depth) {
    print_indent(depth);
    printf("%s {\n", node->name[0] ? node->name : "/");
    for (u32 i = 0; i < node->prop_count; ++i)
        dump_prop(&node->props[i], depth + 1);
    for (dtb_node_t *c = node->children; c; c = c->next_sibling)
        dump_node(c, depth + 1);
    print_indent(depth);
    printf("};\n");
}

void dtb_dump(const dtb_tree_t *tree) {
    if (tree && tree->root) dump_node(tree->root, 0);
}
