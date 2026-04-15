#!/usr/bin/env python3
"""
mkfs.py — build a LEGF filesystem image and write it at sector 0 of disk.img.

Usage:
    mkfs.py --disk data/disk.img --blob NAME:PATH[:FLAGS] [--blob ...]

PATH may be an ELF executable or a raw binary.
  - ELF:  converted to LLF on the fly; blob.size = LLF file size.
          The kernel reads the LLF and derives mapped size from segment headers.
  - raw:  stored verbatim; blob.size = file size.

FLAGS: comma-separated flag names (currently only 'exec').
"""

import argparse
import os
import struct
import sys

from llf import parse_elf, to_llf_bytes

FS_MAGIC     = 0x4C454746  # "LEGF"
FS_FLAG_EXEC = 0x0001

ELF_MAGIC = b"\x7fELF"


def align_up(n: int, align: int) -> int:
    return (n + align - 1) & ~(align - 1)


def load_blob(path: str) -> dict:
    """
    Returns a dict with keys: data (bytes), file_size (int), mem_size (int).
    For ELF: data is the LLF-encoded binary; mem_size is the full VA span.
    For raw: data is the file contents; mem_size == file_size.
    """
    with open(path, "rb") as f:
        raw = f.read()

    if not raw.startswith(ELF_MAGIC):
        return {"data": raw, "file_size": len(raw), "mem_size": len(raw)}

    entry, segments = parse_elf(path)
    data = to_llf_bytes(entry, segments)

    load_va   = min(s.vaddr for s in segments)
    image_end = max(s.vaddr + s.memsz for s in segments)

    return {
        "data":      data,
        "file_size": len(data),
        "mem_size":  image_end - load_va,
    }


def build_fs(blobs: list[dict]) -> bytes:
    blob_count  = len(blobs)
    header_size = 8
    desc_size   = 16 * blob_count

    name_data    = b""
    name_offsets = []
    for blob in blobs:
        name_offsets.append(len(name_data))
        name_data += blob["name"].encode()

    names_start = header_size + desc_size
    data_start  = align_up(names_start + len(name_data), 512)

    blob_offsets = []
    cur = data_start
    for blob in blobs:
        blob_offsets.append(cur)
        cur = align_up(cur + len(blob["data"]), 512)

    img = bytearray()
    img += struct.pack("<II", FS_MAGIC, blob_count)

    for i, blob in enumerate(blobs):
        img += struct.pack(
            "<IIIHH",
            blob_offsets[i],
            blob["file_size"],      # on-disk size; kernel reads this many bytes
            name_offsets[i],
            len(blob["name"].encode()),
            blob["flags"],
        )

    img += name_data
    img += b"\x00" * (data_start - len(img))

    for blob in blobs:
        img += blob["data"]
        img += b"\x00" * (align_up(len(blob["data"]), 512) - len(blob["data"]))

    return bytes(img)


def parse_blob_arg(arg: str) -> tuple[str, int, str]:
    parts = arg.split(":")
    if len(parts) < 2:
        print(f"error: --blob must be NAME:PATH[:FLAGS], got '{arg}'", file=sys.stderr)
        sys.exit(1)
    name  = parts[0]
    path  = parts[1]
    flags = 0
    if len(parts) >= 3:
        for f in parts[2].split(","):
            f = f.strip().lower()
            if f == "exec":
                flags |= FS_FLAG_EXEC
            elif f:
                print(f"warning: unknown flag '{f}' for blob '{name}'", file=sys.stderr)
    return name, flags, path


def main() -> None:
    p = argparse.ArgumentParser(description="Build a LEGF filesystem image")
    p.add_argument("--disk",      required=True, help="Path to disk.img")
    p.add_argument("--blob",      required=True, action="append",
                   metavar="NAME:PATH[:FLAGS]", help="Blob to include (repeatable)")
    p.add_argument("--disk-size", type=int, default=64 * 1024 * 1024,
                   help="Disk image size in bytes (default: 64 MiB)")
    args = p.parse_args()

    blobs = []
    for blob_arg in args.blob:
        name, flags, path = parse_blob_arg(blob_arg)
        blob = load_blob(path)
        blob["name"]  = name
        blob["flags"] = flags
        print(f"mkfs: blob '{name}'  {blob['file_size']} bytes on disk"
              + (f" / {blob['mem_size']} bytes mapped" if blob["mem_size"] != blob["file_size"] else "")
              + f"  flags=0x{flags:04x}  ({path})")
        blobs.append(blob)

    fs_image = build_fs(blobs)

    disk_dir = os.path.dirname(args.disk)
    if disk_dir:
        os.makedirs(disk_dir, exist_ok=True)

    if not os.path.exists(args.disk):
        print(f"mkfs: creating blank disk image ({args.disk_size // (1024*1024)} MiB): {args.disk}")
        with open(args.disk, "wb") as f:
            f.write(b"\x00" * args.disk_size)

    with open(args.disk, "r+b") as f:
        f.seek(0)
        f.write(fs_image)

    print(f"mkfs: wrote FS image ({len(fs_image)} bytes, {len(fs_image) // 512} sectors) to {args.disk}")


if __name__ == "__main__":
    main()