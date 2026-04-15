import struct
from dataclasses import dataclass

ELF_MAGIC = b"\x7fELF"

PT_LOAD = 1

LLF_MAGIC   = 0x4C4C4646
LLF_VERSION = 0x00010000

LLF_SEG_NULL   = 0
LLF_SEG_LOAD   = 1
LLF_SEG_NOBITS = 2

HEADER_SIZE = 24  # magic(4) + version(4) + entry(4) + ph_off(4) + ph_count(4) + flags(4)
PHDR_SIZE   = 28  # type(4) + flags(4) + vaddr(4) + memsz(4) + filesz(4) + offset(4) + align(4)


@dataclass
class Segment:
    type:   int
    flags:  int
    vaddr:  int
    memsz:  int
    filesz: int
    data:   bytes


def parse_elf(path: str) -> tuple[int, list[Segment]]:
    with open(path, "rb") as f:
        elf = f.read()

    if not elf.startswith(ELF_MAGIC):
        raise ValueError("not an ELF file")

    # ELF32 little-endian header fields
    (e_entry, e_phoff) = struct.unpack_from("<II", elf, 0x18)
    (e_phentsize, e_phnum) = struct.unpack_from("<HH", elf, 0x2a)

    segments = []
    for i in range(e_phnum):
        base = e_phoff + i * e_phentsize
        p_type, p_offset, p_vaddr, _, p_filesz, p_memsz, p_flags, p_align = \
            struct.unpack_from("<IIIIIIII", elf, base)

        if p_type != PT_LOAD or p_memsz == 0:
            continue

        if p_filesz > 0:
            segments.append(Segment(
                type   = LLF_SEG_LOAD,
                flags  = p_flags,
                vaddr  = p_vaddr,
                memsz  = p_memsz,
                filesz = p_filesz,
                data   = elf[p_offset:p_offset + p_filesz],
            ))
        else:
            segments.append(Segment(
                type   = LLF_SEG_NOBITS,
                flags  = p_flags,
                vaddr  = p_vaddr,
                memsz  = p_memsz,
                filesz = 0,
                data   = b"",
            ))

    return e_entry, segments


def write_llf(path_out: str, entry: int, segments: list[Segment]) -> None:
    ph_count  = len(segments)
    data_base = HEADER_SIZE + ph_count * PHDR_SIZE

    # Pre-compute file offsets for each segment's data
    data_offset = data_base
    offsets = []
    for s in segments:
        offsets.append(data_offset if s.filesz > 0 else 0)
        data_offset += s.filesz

    blob = bytearray()

    # Header
    blob += struct.pack("<IIIIII",
        LLF_MAGIC,
        LLF_VERSION,
        entry,
        HEADER_SIZE,   # ph_off: program headers start immediately after header
        ph_count,
        0,             # flags: reserved
    )

    # Program headers
    for i, s in enumerate(segments):
        blob += struct.pack("<IIIIIII",
            s.type,
            s.flags,
            s.vaddr,
            s.memsz,
            s.filesz,
            offsets[i],
            0,  # align: not used by kernel loader
        )

    # Segment data
    for s in segments:
        if s.filesz > 0:
            blob += s.data

    with open(path_out, "wb") as f:
        f.write(blob)


def to_llf_bytes(entry: int, segments: list[Segment]) -> bytes:
    """Like write_llf but returns bytes instead of writing to a file."""
    ph_count  = len(segments)
    data_base = HEADER_SIZE + ph_count * PHDR_SIZE

    data_offset = data_base
    offsets = []
    for s in segments:
        offsets.append(data_offset if s.filesz > 0 else 0)
        data_offset += s.filesz

    blob = bytearray()
    blob += struct.pack("<IIIIII", LLF_MAGIC, LLF_VERSION, entry, HEADER_SIZE, ph_count, 0)
    for i, s in enumerate(segments):
        blob += struct.pack("<IIIIIII", s.type, s.flags, s.vaddr, s.memsz, s.filesz, offsets[i], 0)
    for s in segments:
        if s.filesz > 0:
            blob += s.data

    return bytes(blob)