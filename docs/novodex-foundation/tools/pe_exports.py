#!/usr/bin/env python3
"""Print the named exports of a PE32 image."""

import argparse
import struct
from pathlib import Path


def pe_named_exports(path) -> list[tuple[int, str]]:
    data = Path(path).read_bytes()

    def unpack(fmt, offset, description):
        size = struct.calcsize(fmt)
        if offset < 0 or offset + size > len(data):
            raise ValueError(f"truncated {description}")
        return struct.unpack_from(fmt, data, offset)

    (pe_offset,) = unpack("<I", 0x3C, "DOS header")
    if pe_offset + 24 > len(data) or data[pe_offset:pe_offset + 4] != b"PE\0\0":
        raise ValueError("not a PE image")

    _, section_count, _, _, _, optional_size, _ = unpack(
        "<HHIIIHH", pe_offset + 4, "COFF header"
    )
    optional_offset = pe_offset + 24
    if optional_size < 104:
        raise ValueError("truncated PE32 optional header")
    (magic,) = unpack("<H", optional_offset, "optional header")
    if magic != 0x10B:
        raise ValueError("optional header is not PE32")
    export_rva, export_size = unpack(
        "<II", optional_offset + 96, "export data directory"
    )

    sections = []
    section_offset = optional_offset + optional_size
    for index in range(section_count):
        offset = section_offset + index * 40
        virtual_size, virtual_address, raw_size, raw_offset = unpack(
            "<IIII", offset + 8, "section header"
        )
        sections.append((virtual_address, virtual_size, raw_size, raw_offset))

    def rva_mapping(rva, size=1):
        for virtual_address, virtual_size, raw_size, raw_offset in sections:
            delta = rva - virtual_address
            if 0 <= delta < max(virtual_size, raw_size):
                if delta + size > raw_size or raw_offset + delta + size > len(data):
                    raise ValueError(f"RVA 0x{rva:x} is not backed by file data")
                return raw_offset + delta, raw_offset + raw_size
        raise ValueError(f"RVA 0x{rva:x} does not map to a section")

    def rva_to_offset(rva, size=1):
        return rva_mapping(rva, size)[0]

    def read_c_string(rva):
        offset, raw_end = rva_mapping(rva)
        end = data.find(b"\0", offset, raw_end)
        if end < 0:
            raise ValueError(f"unterminated export name at RVA 0x{rva:x}")
        try:
            return data[offset:end].decode("ascii")
        except UnicodeDecodeError as error:
            raise ValueError(f"non-ASCII export name at RVA 0x{rva:x}") from error

    if export_rva == 0 or export_size == 0:
        return []

    export_offset = rva_to_offset(export_rva, 40)
    fields = unpack("<IIHHIIIIIII", export_offset, "export directory")
    ordinal_base = fields[5]
    name_count = fields[7]
    names_rva = fields[9]
    ordinals_rva = fields[10]

    exports = []
    for index in range(name_count):
        (name_rva,) = unpack(
            "<I", rva_to_offset(names_rva + index * 4, 4), "export name RVA"
        )
        (ordinal_index,) = unpack(
            "<H", rva_to_offset(ordinals_rva + index * 2, 2), "export ordinal"
        )
        exports.append((ordinal_base + ordinal_index, read_c_string(name_rva)))
    return exports


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", help="PE32 image to inspect")
    args = parser.parse_args()
    try:
        exports = pe_named_exports(args.path)
    except (OSError, ValueError) as error:
        parser.exit(2, f"error: {error}\n")
    for ordinal, name in exports:
        print(ordinal, name)


if __name__ == "__main__":
    main()
