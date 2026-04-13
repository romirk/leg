//
// Created by Romir Kulshrestha on 13/04/2026.
//

#ifndef LEG_FS_H
#define LEG_FS_H

#include "types.h"

#define FS_MAGIC     0x4C454746u // "LEGF"
#define FS_FLAG_EXEC 0x0001u     // blob is executable

// On-disk blob descriptor — 16 bytes, packed.
// All offsets are relative to the start of the FS image.
// Blob data offsets must be 512-byte aligned.
typedef struct [[gnu::packed]] {
    u32 offset;      // byte offset of blob data from FS image start
    u32 size;        // blob size in bytes
    u32 name_offset; // byte offset of name within the name_data region
    u16 name_size;   // name length in bytes (no null terminator on disk)
    u16 flags;
} fs_blob_t;

// Mount the filesystem starting at disk sector `start_sector`.
// Reads the header, descriptor table, and name data into kernel memory.
// Returns true on success.
bool fs_mount(u32 start_sector);

// Find a blob by name. Returns a pointer into the in-memory descriptor table,
// or nullptr if not found.
[[gnu::pure]]
const fs_blob_t *fs_find(const char *name);

// Read a blob's data into buf (caller must provide at least blob->size bytes).
// Returns true on success.
bool fs_read(const fs_blob_t *blob, void *buf);

// Returns the number of blobs in the mounted filesystem.
[[gnu::pure]]
u32 fs_blob_count(void);

#endif // LEG_FS_H