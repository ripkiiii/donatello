#!/usr/bin/env python3
"""mkdisk.py — pack files into Donatello's flat-file-table disk image.

Runs on the HOST (this is a build-time tool, not kernel code) — it's the
"write" half of a filesystem that has no write path at runtime (see fs.h).

Format (mirrors fs.h/fs.c on the kernel side exactly):
    sector 0     = file table: up to 16 entries, 32 bytes each
                   { name[24] (NUL-padded ASCII), start_lba (u32 LE),
                     size_bytes (u32 LE) }
                   end of table = first entry whose name[0] is 0x00
    sector 1..N  = file contents, each starting at its own sector boundary
                   (padded with zeros up to the next 512-byte boundary so
                   the NEXT file always starts on a fresh sector)

Usage:
    python3 mkdisk.py <output.img> name1=path1 name2=path2 ...
"""
import struct
import sys

SECTOR = 512
ENTRY_SIZE = 32
NAME_MAX = 24
MAX_ENTRIES = SECTOR // ENTRY_SIZE  # 16


def build(out_path, files):
    if len(files) > MAX_ENTRIES:
        sys.exit(f"mkdisk: too many files ({len(files)}), max is {MAX_ENTRIES}")

    table = bytearray(SECTOR)
    data = bytearray()
    next_lba = 1  # sector 0 is the table itself

    for i, (name, path) in enumerate(files):
        name_bytes = name.encode("ascii")
        if len(name_bytes) >= NAME_MAX:
            sys.exit(f"mkdisk: name too long (max {NAME_MAX - 1}): {name}")

        with open(path, "rb") as f:
            content = f.read()

        entry = name_bytes.ljust(NAME_MAX, b"\x00")
        entry += struct.pack("<II", next_lba, len(content))
        table[i * ENTRY_SIZE:(i + 1) * ENTRY_SIZE] = entry

        padded_len = (len(content) + SECTOR - 1) // SECTOR * SECTOR
        data += content + b"\x00" * (padded_len - len(content))
        next_lba += padded_len // SECTOR

    with open(out_path, "wb") as f:
        f.write(table)
        f.write(data)

    print(f"mkdisk: wrote {out_path} ({len(table) + len(data)} bytes, "
          f"{len(files)} file(s))")
    for name, path in files:
        print(f"  {name:<24} <- {path}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(__doc__)

    out = sys.argv[1]
    pairs = []
    for arg in sys.argv[2:]:
        if "=" not in arg:
            sys.exit(f"mkdisk: expected name=path, got: {arg}")
        name, path = arg.split("=", 1)
        pairs.append((name, path))

    build(out, pairs)
