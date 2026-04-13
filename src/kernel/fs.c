#include "kernel/fs.h"

#include "kernel/dev/blk.h"
#include "kernel/logs.h"
#include "kernel/mem/alloc.h"
#include "libc/builtins.h"
#include "libc/string.h"

// On-disk image header (8 bytes), immediately followed by fs_blob_t[blob_count].
typedef struct [[gnu::packed]] {
    u32 magic;
    u32 blob_count;
} fs_header_t;

static u32       g_start_sector;
static u32       g_blob_count;
static fs_blob_t *g_blobs; // kmalloc'd, g_blob_count entries
static char      *g_names; // kmalloc'd, raw name bytes (null-terminated per entry)

static bool read_sectors(u32 sector, u32 count, void *buf) {
    return blk_read(sector, count, buf);
}

[[gnu::pure]]
u32 fs_blob_count(void) {
    return g_blob_count;
}

bool fs_mount(const u32 start_sector) {
    g_blob_count = 0;
    g_blobs      = nullptr;
    g_names      = nullptr;

    // ── Pass 1: read first sector, validate header ────────────────────────────
    u8 hdr_buf[512];
    if (!read_sectors(start_sector, 1, hdr_buf)) {
        err("fs: failed to read header");
        return false;
    }

    const fs_header_t *hdr = (const fs_header_t *) hdr_buf;
    if (hdr->magic != FS_MAGIC) {
        err("fs: bad magic 0x%x (expected 0x%x)", hdr->magic, FS_MAGIC);
        return false;
    }

    g_start_sector = start_sector;
    g_blob_count   = hdr->blob_count;

    if (g_blob_count == 0) {
        info("fs: mounted (empty) at sector %u", start_sector);
        return true;
    }

    // ── Pass 2: read header + full descriptor table ───────────────────────────
    const u32 desc_end  = sizeof(fs_header_t) + g_blob_count * sizeof(fs_blob_t);
    const u32 desc_secs = (desc_end + 511) / 512;

    u8 *desc_buf = kmalloc(desc_secs * 512);
    if (!desc_buf) { err("fs: OOM (desc)"); return false; }

    if (!read_sectors(start_sector, desc_secs, desc_buf)) {
        err("fs: failed to read descriptor table");
        kfree(desc_buf);
        return false;
    }

    // Copy descriptors to permanent storage.
    g_blobs = kmalloc(g_blob_count * sizeof(fs_blob_t));
    if (!g_blobs) { kfree(desc_buf); err("fs: OOM (blobs)"); return false; }
    memcpy(g_blobs, desc_buf + sizeof(fs_header_t), g_blob_count * sizeof(fs_blob_t));
    kfree(desc_buf);

    // ── Pass 3: read name data ────────────────────────────────────────────────
    // Determine the extent of the name region by scanning all descriptors.
    u32 names_size = 0;
    for (u32 i = 0; i < g_blob_count; i++) {
        const u32 end = g_blobs[i].name_offset + g_blobs[i].name_size;
        if (end > names_size) names_size = end;
    }

    const u32 names_start = desc_end; // names immediately follow descriptor table
    const u32 names_end   = names_start + names_size;
    const u32 names_secs  = (names_end + 511) / 512;

    u8 *name_buf = kmalloc(names_secs * 512);
    if (!name_buf) { kfree(g_blobs); err("fs: OOM (names buf)"); return false; }

    if (!read_sectors(start_sector, names_secs, name_buf)) {
        err("fs: failed to read name data");
        kfree(g_blobs);
        kfree(name_buf);
        return false;
    }

    // Copy name data; add a null terminator after each name for safe lookup.
    g_names = kmalloc(names_size + g_blob_count); // +1 null per entry
    if (!g_names) {
        kfree(g_blobs);
        kfree(name_buf);
        err("fs: OOM (names)");
        return false;
    }
    memcpy(g_names, name_buf + names_start, names_size);
    kfree(name_buf);

    info("fs: mounted at sector %u — %u blobs", start_sector, g_blob_count);
    return true;
}

[[gnu::pure]]
const fs_blob_t *fs_find(const char *name) {
    const u32 name_len = (u32) strlen(name);
    for (u32 i = 0; i < g_blob_count; i++) {
        const fs_blob_t *b = &g_blobs[i];
        if (b->name_size == name_len &&
            strncmp(g_names + b->name_offset, name, name_len) == 0)
            return b;
    }
    return nullptr;
}

bool fs_read(const fs_blob_t *blob, void *buf) {
    // Blob offsets are 512-aligned (guaranteed by mkfs).
    const u32 sector = g_start_sector + blob->offset / 512;
    const u32 count  = (blob->size + 511) / 512;
    return blk_read(sector, count, buf);
}