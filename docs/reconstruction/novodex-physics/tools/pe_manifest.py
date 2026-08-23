#!/usr/bin/env python3
"""Inventory the structure of a PE32 image from the file format alone."""

import argparse
import hashlib
import json
import struct
from pathlib import Path


SCHEMA_VERSION = 1

IMAGE_SCN_MEM_EXECUTE = 0x20000000

DIRECTORY_NAMES = (
    "export", "import", "resource", "exception", "security", "basereloc", "debug",
    "architecture", "global_ptr", "tls", "load_config", "bound_import", "iat",
    "delay_import", "clr_runtime", "reserved",
)

RELOCATION_TYPES = {0: "ABSOLUTE", 1: "HIGH", 2: "LOW", 3: "HIGHLOW", 4: "HIGHADJ", 10: "DIR64"}

# On PE32 only HIGHLOW patches a whole 32-bit address, so only it identifies a
# pointer-sized value; the other kinds patch fragments of one.
POINTER_RELOCATION = 3

# Shortest run the independent string scan will report, and the byte range it
# treats as printable. The published scan criterion interpolates both, so the
# prose in the manifest cannot drift from the code.
MIN_STRING_LENGTH = 4
PRINTABLE_MIN = 0x20
PRINTABLE_MAX = 0x7E


def hex16(value) -> str:
    return f"0x{value:04x}"


def hex32(value) -> str:
    return f"0x{value:08x}"


def unpack(data, fmt, offset, description):
    size = struct.calcsize(fmt)
    if offset < 0 or offset + size > len(data):
        raise ValueError(f"truncated {description}")
    return struct.unpack_from(fmt, data, offset)


class _Image:
    """A bounds-checked view of a PE32 file and its section table."""

    def __init__(self, data, sections):
        self.data = data
        self.sections = sections

    def unpack(self, fmt, offset, description):
        return unpack(self.data, fmt, offset, description)

    def section_of(self, rva):
        """Return the section mapping an RVA, or None when nothing maps it."""
        for section in self.sections:
            delta = rva - section["rva"]
            if 0 <= delta < max(section["virtual_size"], section["raw_size"]):
                return section
        return None

    def rva_mapping(self, rva, size=1):
        """Return the file offset of an RVA and the end of its section's raw data."""
        section = self.section_of(rva)
        if section is None:
            raise ValueError(f"RVA {hex32(rva)} does not map to a section")
        delta = rva - section["rva"]
        raw_end = section["raw_offset"] + section["raw_size"]
        if delta + size > section["raw_size"] or section["raw_offset"] + delta + size > len(self.data):
            raise ValueError(f"RVA {hex32(rva)} is not backed by file data")
        return section["raw_offset"] + delta, raw_end

    def rva_to_offset(self, rva, size=1):
        return self.rva_mapping(rva, size)[0]

    def read_c_string(self, rva, description):
        offset, raw_end = self.rva_mapping(rva)
        end = self.data.find(b"\0", offset, raw_end)
        if end < 0:
            raise ValueError(f"unterminated {description} at RVA {hex32(rva)}")
        try:
            return self.data[offset:end].decode("ascii")
        except UnicodeDecodeError as error:
            raise ValueError(f"non-ASCII {description} at RVA {hex32(rva)}") from error


def _parse_exports(image, rva, size):
    if rva == 0 or size == 0:
        return None
    offset = image.rva_to_offset(rva, 40)
    (_, timestamp, _, _, name_rva, ordinal_base, function_count, name_count,
     address_table, name_table, ordinal_table) = image.unpack(
        "<IIHHIIIIIII", offset, "export directory")

    names = {}
    for index in range(name_count):
        (entry_rva,) = image.unpack(
            "<I", image.rva_to_offset(name_table + index * 4, 4), "export name RVA")
        (slot,) = image.unpack(
            "<H", image.rva_to_offset(ordinal_table + index * 2, 2), "export ordinal")
        names[slot] = image.read_c_string(entry_rva, "export name")

    entries = []
    for index in range(function_count):
        (function_rva,) = image.unpack(
            "<I", image.rva_to_offset(address_table + index * 4, 4), "export address")
        if function_rva == 0:
            continue
        forwarder = None
        if rva <= function_rva < rva + size:
            forwarder = image.read_c_string(function_rva, "export forwarder")
        entries.append({
            "ordinal": ordinal_base + index,
            "name": names.get(index),
            "rva": function_rva,
            "rva_hex": hex32(function_rva),
            "forwarder": forwarder,
        })

    return {
        "directory_rva": rva,
        "directory_rva_hex": hex32(rva),
        "name": image.read_c_string(name_rva, "export library name"),
        "timestamp": timestamp,
        "ordinal_base": ordinal_base,
        "named_count": sum(1 for entry in entries if entry["name"] is not None),
        "entries": entries,
    }


def _parse_imports(image, rva, size):
    if rva == 0 or size == 0:
        return []
    imports = []
    descriptor = 0
    while True:
        offset = image.rva_to_offset(rva + descriptor * 20, 20)
        lookup_rva, _, _, name_rva, iat_rva = image.unpack("<IIIII", offset, "import descriptor")
        if lookup_rva == 0 and name_rva == 0 and iat_rva == 0:
            break
        dll = image.read_c_string(name_rva, "import library name")
        thunk_rva = lookup_rva or iat_rva
        slot = 0
        while True:
            (thunk,) = image.unpack(
                "<I", image.rva_to_offset(thunk_rva + slot * 4, 4), "import thunk")
            if thunk == 0:
                break
            entry = {"dll": dll, "name": None, "ordinal": None, "hint": None,
                     "hint_name_rva": None, "hint_name_rva_hex": None,
                     "iat_rva": iat_rva + slot * 4, "iat_rva_hex": hex32(iat_rva + slot * 4)}
            if thunk & 0x80000000:
                entry["ordinal"] = thunk & 0xFFFF
            else:
                (entry["hint"],) = image.unpack(
                    "<H", image.rva_to_offset(thunk, 2), "import hint")
                entry["name"] = image.read_c_string(thunk + 2, "import name")
                entry["hint_name_rva"] = thunk
                entry["hint_name_rva_hex"] = hex32(thunk)
            imports.append(entry)
            slot += 1
        descriptor += 1
    return imports


def _parse_relocations(image, rva, size):
    """Read every fixup in file order, keeping the ABSOLUTE padding a linker emits."""
    if rva == 0 or size == 0:
        return []
    entries = []
    consumed = 0
    while consumed < size:
        block_rva = rva + consumed
        page_rva, block_size = image.unpack(
            "<II", image.rva_to_offset(block_rva, 8), "base relocation block")
        if block_size == 0:
            raise ValueError(
                f"empty base relocation block at RVA {hex32(block_rva)} abandons "
                f"{size - consumed} bytes of the directory")
        if block_size < 8 or consumed + block_size > size:
            raise ValueError(f"truncated base relocation block at RVA {hex32(block_rva)}")
        count = (block_size - 8) // 2
        offset = image.rva_to_offset(block_rva + 8, count * 2)
        for index in range(count):
            (word,) = image.unpack("<H", offset + index * 2, "base relocation entry")
            kind = word >> 12
            entry_rva = page_rva + (word & 0xFFF)
            entries.append({
                "rva": entry_rva,
                "rva_hex": hex32(entry_rva),
                "type": kind,
                "type_name": RELOCATION_TYPES.get(kind, "UNKNOWN"),
                "block_rva": page_rva,
                "block_rva_hex": hex32(page_rva),
            })
        consumed += block_size
    return entries


def _read_resource_name(image, rva):
    (length,) = image.unpack("<H", image.rva_to_offset(rva, 2), "resource name length")
    offset = image.rva_to_offset(rva + 2, length * 2)
    return image.data[offset:offset + length * 2].decode("utf-16-le")


def _parse_resources(image, rva, size):
    """Walk the type/name/language directories and return their data leaves."""
    if rva == 0 or size == 0:
        return []
    leaves = []

    def walk(directory_rva, depth, path):
        if depth > 3:
            raise ValueError(
                f"resource tree is deeper than three levels at RVA {hex32(directory_rva)}")
        offset = image.rva_to_offset(directory_rva, 16)
        *_, named_count, id_count = image.unpack("<IIHHHH", offset, "resource directory")
        for index in range(named_count + id_count):
            identifier, child = image.unpack(
                "<II", offset + 16 + index * 8, "resource directory entry")
            if identifier & 0x80000000:
                key = _read_resource_name(image, rva + (identifier & 0x7FFFFFFF))
            else:
                key = identifier
            if child & 0x80000000:
                walk(rva + (child & 0x7FFFFFFF), depth + 1, path + [key])
                continue
            data_rva, data_size, codepage, _ = image.unpack(
                "<IIII", image.rva_to_offset(rva + child, 16), "resource data entry")
            image.rva_mapping(data_rva, data_size)
            type_id, name_id, language_id = (path + [key, None, None])[:3]
            leaves.append({
                "type": type_id,
                "name": name_id,
                "language": language_id,
                "rva": data_rva,
                "rva_hex": hex32(data_rva),
                "size": data_size,
                "codepage": codepage,
            })

    walk(rva, 1, [])
    return sorted(leaves, key=lambda leaf: leaf["rva"])


def _mapped_extent(section):
    """Bytes of a section that are both mapped and present in the file."""
    return min(section["virtual_size"], section["raw_size"])


def _scan_ascii(blob, section):
    strings = []
    start = None
    for index, byte in enumerate(blob):
        if PRINTABLE_MIN <= byte <= PRINTABLE_MAX:
            if start is None:
                start = index
            continue
        if byte == 0 and start is not None and index - start >= MIN_STRING_LENGTH:
            rva = section["rva"] + start
            strings.append({
                "rva": rva,
                "rva_hex": hex32(rva),
                "section": section["name"],
                "encoding": "ascii",
                "length": index - start,
                "size": index - start + 1,
                "value": blob[start:index].decode("ascii"),
            })
        start = None
    return strings


def _scan_utf16(blob, section):
    strings = []
    start = None
    for index in range(0, len(blob) - 1, 2):
        low, high = blob[index], blob[index + 1]
        if high == 0 and PRINTABLE_MIN <= low <= PRINTABLE_MAX:
            if start is None:
                start = index
            continue
        if low == 0 and high == 0 and start is not None \
                and (index - start) // 2 >= MIN_STRING_LENGTH:
            rva = section["rva"] + start
            strings.append({
                "rva": rva,
                "rva_hex": hex32(rva),
                "section": section["name"],
                "encoding": "utf-16le",
                "length": (index - start) // 2,
                "size": index - start + 2,
                "value": blob[start:index].decode("utf-16-le"),
            })
        start = None
    return strings


def _scan_strings(image, scanned):
    strings = []
    for section in scanned:
        start = section["raw_offset"]
        blob = image.data[start:start + _mapped_extent(section)]
        strings.extend(_scan_ascii(blob, section))
        strings.extend(_scan_utf16(blob, section))
    return sorted(strings, key=lambda entry: (entry["rva"], entry["encoding"]))


def _scan_pointers(image, image_base, relocated_sites, scanned):
    """Collect pointer-sized values that address the mapped image.

    Every naturally aligned slot of the scanned sections is examined, and every
    HIGHLOW relocation site is examined again so that pointers embedded at
    unaligned offsets inside instructions are not missed. A relocated site is
    always reported, even when its value leaves the image, because dropping one
    would hide a malformed fixup.
    """
    found = {}

    def record(rva, section, relocated):
        offset, _ = image.rva_mapping(rva, 4)
        (value,) = image.unpack("<I", offset, "pointer value")
        target = value - image_base
        target_section = image.section_of(target) if target > 0 else None
        if target_section is None and not relocated:
            return
        found[rva] = {
            "rva": rva,
            "rva_hex": hex32(rva),
            "section": section["name"],
            "value": value,
            "value_hex": hex32(value),
            "target_rva": target if target_section else None,
            "target_rva_hex": hex32(target) if target_section else None,
            "target_section": target_section["name"] if target_section else None,
            "relocated": relocated,
        }

    for section in scanned:
        for delta in range(0, max(_mapped_extent(section) - 3, 0), 4):
            record(section["rva"] + delta, section, False)
    for rva in sorted(relocated_sites):
        section = image.section_of(rva)
        if section is None:
            raise ValueError(f"relocation site RVA {hex32(rva)} does not map to a section")
        record(rva, section, True)
    return [found[rva] for rva in sorted(found)]


def build_manifest(path) -> dict:
    """Read a PE32 image and return its structural manifest."""
    path = Path(path)
    data = path.read_bytes()

    (pe_offset,) = unpack(data, "<I", 0x3C, "DOS header")
    if pe_offset + 24 > len(data) or data[pe_offset:pe_offset + 4] != b"PE\0\0":
        raise ValueError("not a PE image")

    machine, section_count, timestamp, _, _, optional_size, characteristics = unpack(
        data, "<HHIIIHH", pe_offset + 4, "COFF header")
    optional_offset = pe_offset + 24
    if optional_size < 96:
        raise ValueError("truncated PE32 optional header")
    (magic,) = unpack(data, "<H", optional_offset, "optional header")
    if magic != 0x10B:
        raise ValueError("optional header is not PE32")
    (entry_point,) = unpack(data, "<I", optional_offset + 16, "entry point")
    (image_base,) = unpack(data, "<I", optional_offset + 28, "image base")
    section_alignment, file_alignment = unpack(
        data, "<II", optional_offset + 32, "section alignments")
    (size_of_image,) = unpack(data, "<I", optional_offset + 56, "image size")
    (directory_count,) = unpack(data, "<I", optional_offset + 92, "data directory count")
    if optional_size < 96 + directory_count * 8:
        raise ValueError("truncated PE32 data directory")

    data_directories = []
    for index in range(directory_count):
        directory_rva, directory_size = unpack(
            data, "<II", optional_offset + 96 + index * 8, "data directory")
        data_directories.append({
            "index": index,
            "name": DIRECTORY_NAMES[index] if index < len(DIRECTORY_NAMES) else "unknown",
            "rva": directory_rva,
            "rva_hex": hex32(directory_rva),
            "size": directory_size,
        })

    sections = []
    table_offset = optional_offset + optional_size
    for index in range(section_count):
        offset = table_offset + index * 40
        (raw_name,) = unpack(data, "<8s", offset, "section header")
        virtual_size, rva, raw_size, raw_offset = unpack(
            data, "<IIII", offset + 8, "section header")
        (flags,) = unpack(data, "<I", offset + 36, "section header")
        name = raw_name.rstrip(b"\0").decode("ascii")
        if raw_size and raw_offset + raw_size > len(data):
            raise ValueError(f"section {name} raw data extends past the end of the file")
        sections.append({
            "name": name,
            "rva": rva,
            "rva_hex": hex32(rva),
            "virtual_size": virtual_size,
            "raw_offset": raw_offset,
            "raw_size": raw_size,
            "characteristics": flags,
            "characteristics_hex": hex32(flags),
            "executable": bool(flags & IMAGE_SCN_MEM_EXECUTE),
            "sha256": hashlib.sha256(data[raw_offset:raw_offset + raw_size]).hexdigest(),
        })

    image = _Image(data, sections)

    executable_intervals = []
    for section in sections:
        if not section["executable"]:
            continue
        extent = _mapped_extent(section)
        executable_intervals.append({
            "section": section["name"],
            "rva": section["rva"],
            "rva_hex": section["rva_hex"],
            "end_rva": section["rva"] + extent,
            "end_rva_hex": hex32(section["rva"] + extent),
            "size": extent,
            "raw_offset": section["raw_offset"],
        })

    def directory(index):
        entry = data_directories[index] if index < len(data_directories) else None
        return (entry["rva"], entry["size"]) if entry else (0, 0)

    exports = _parse_exports(image, *directory(0))
    imports = _parse_imports(image, *directory(1))
    relocations = _parse_relocations(image, *directory(5))
    resource_rva, resource_size = directory(2)
    resources = _parse_resources(image, resource_rva, resource_size)

    # The census scans every mapped section that is not the resource section.
    resource_section = image.section_of(resource_rva) if resource_rva else None
    scanned = [section for section in sections if section is not resource_section]
    relocated_sites = {entry["rva"] for entry in relocations if entry["type"] == POINTER_RELOCATION}

    return {
        "schema_version": SCHEMA_VERSION,
        "image": {
            "name": path.name,
            "size": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
            "machine": machine,
            "machine_hex": hex16(machine),
            "magic": magic,
            "magic_hex": hex16(magic),
            "characteristics": characteristics,
            "characteristics_hex": hex32(characteristics),
            "timestamp": timestamp,
            "timestamp_hex": hex32(timestamp),
            "image_base": image_base,
            "image_base_hex": hex32(image_base),
            "entry_point": entry_point,
            "entry_point_hex": hex32(entry_point),
            "section_alignment": section_alignment,
            "file_alignment": file_alignment,
            "size_of_image": size_of_image,
            "size_of_image_hex": hex32(size_of_image),
        },
        "data_directories": data_directories,
        "sections": sections,
        "executable_intervals": executable_intervals,
        "executable_bytes": sum(interval["size"] for interval in executable_intervals),
        "exports": exports,
        "imports": imports,
        "relocations": relocations,
        "resources": resources,
        "string_scan": {
            "min_length": MIN_STRING_LENGTH,
            "encodings": ["ascii", "utf-16le"],
            "criterion": (
                f"maximal runs of at least {MIN_STRING_LENGTH} printable characters "
                f"(0x{PRINTABLE_MIN:02x}-0x{PRINTABLE_MAX:02x}) that are NUL-terminated within "
                "the scanned extent of a mapped non-resource section; a run closed by any other "
                "byte, or by the end of that extent, is not recorded. utf-16le is additionally "
                "read only at offsets even relative to the section start, and every code unit "
                f"must lie in that same printable range (U+{PRINTABLE_MIN:04X}-U+{PRINTABLE_MAX:04X}), "
                "so a utf-16le run at an odd offset, or one containing any character outside "
                "that range, is not recorded"
            ),
        },
        "strings": _scan_strings(image, scanned),
        "pointers": _scan_pointers(image, image_base, relocated_sites, scanned),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, help="PE32 image to inventory")
    parser.add_argument("--output", required=True, help="manifest JSON to write")
    args = parser.parse_args()

    try:
        manifest = build_manifest(args.input)
        output = Path(args.output)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(manifest, separators=(",", ":")) + "\n",
                          encoding="utf-8", newline="\n")
    except (OSError, ValueError) as error:
        parser.exit(2, f"error: {error}\n")

    exports = manifest["exports"]
    print("pe=written")
    print(f"sha256={manifest['image']['sha256']}")
    print(f"size={manifest['image']['size']}")
    print(f"machine={manifest['image']['machine_hex']} magic={manifest['image']['magic_hex']}")
    print(f"sections={len(manifest['sections'])}")
    print(f"executable_bytes={manifest['executable_bytes']}")
    print(f"exports={exports['named_count'] if exports else 0}")
    print(f"imports={len(manifest['imports'])}")
    print(f"relocations={len(manifest['relocations'])}")
    print(f"resources={len(manifest['resources'])}")
    print(f"strings={len(manifest['strings'])}")
    print(f"pointers={len(manifest['pointers'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
