import hashlib
import json
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import pe_manifest  # noqa: E402


IMAGE_BASE = 0x10000000
TIMESTAMP = 0x41B0C4AD

# File offsets inside the synthesized fixture that the negative tests mutate.
PE_SIGNATURE = 0x80
COFF_HEADER = 0x84
OPTIONAL_SIZE = COFF_HEADER + 16
OPTIONAL_HEADER = 0x98
SECTION_TABLE = OPTIONAL_HEADER + 0xE0
RDATA_RAW_SIZE = SECTION_TABLE + 40 + 16
EXPORT_NAME_POINTERS = 0x630
EXPORT_ALPHA_NAME = 0x650
EXPORT_BETA_TERMINATOR = 0x7FF
IMPORT_LOOKUP_TABLE = 0x730
RELOCATION_BLOCK_PAGE = 0xA00
RELOCATION_BLOCK_SIZE = 0xA04
RESOURCE_LEAF_ENTRY = 0x844
RESOURCE_DATA_RVA = 0x848

# An RVA inside .data's mapped tail, which the section table declares beyond its
# raw data, so nothing in the file backs it.
UNBACKED_TAIL_RVA = 0x5250


def make_pe32(path):
    """Write a small but complete PE32 DLL covering every structure the manifest reads."""
    image = bytearray(0xE00)
    struct.pack_into("<I", image, 0x3C, PE_SIGNATURE)
    image[PE_SIGNATURE:PE_SIGNATURE + 4] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", image, COFF_HEADER, 0x14C, 5, TIMESTAMP, 0, 0, 0xE0, 0x210E)

    struct.pack_into("<H", image, OPTIONAL_HEADER, 0x10B)
    struct.pack_into("<I", image, OPTIONAL_HEADER + 16, 0x1000)
    struct.pack_into("<I", image, OPTIONAL_HEADER + 28, IMAGE_BASE)
    struct.pack_into("<II", image, OPTIONAL_HEADER + 32, 0x1000, 0x200)
    struct.pack_into("<I", image, OPTIONAL_HEADER + 56, 0x6000)
    struct.pack_into("<I", image, OPTIONAL_HEADER + 60, 0x400)
    struct.pack_into("<I", image, OPTIONAL_HEADER + 92, 16)
    directories = OPTIONAL_HEADER + 96
    struct.pack_into("<II", image, directories + 0 * 8, 0x2000, 0x80)
    struct.pack_into("<II", image, directories + 1 * 8, 0x2100, 0x28)
    struct.pack_into("<II", image, directories + 2 * 8, 0x3000, 0x60)
    struct.pack_into("<II", image, directories + 5 * 8, 0x4000, 0x10)
    struct.pack_into("<II", image, directories + 12 * 8, 0x2180, 0x0C)

    # .data mirrors the oracle: a virtual size larger than its raw size, so its
    # tail is mapped but has no file bytes behind it.
    headers = [
        (b".text", 0x40, 0x1000, 0x200, 0x400, 0x60000020),
        (b".rdata", 0x200, 0x2000, 0x200, 0x600, 0x40000040),
        (b".rsrc", 0x100, 0x3000, 0x200, 0x800, 0x40000040),
        (b".reloc", 0x20, 0x4000, 0x200, 0xA00, 0x42000040),
        (b".data", 0x300, 0x5000, 0x200, 0xC00, 0xC0000040),
    ]
    for index, (name, virtual_size, rva, raw_size, raw_offset, flags) in enumerate(headers):
        header = SECTION_TABLE + index * 40
        image[header:header + len(name)] = name
        struct.pack_into("<IIII", image, header + 8, virtual_size, rva, raw_size, raw_offset)
        struct.pack_into("<I", image, header + 36, flags)

    # .text: one aligned relocated pointer and one unaligned relocated pointer.
    struct.pack_into("<I", image, 0x410, IMAGE_BASE + 0x2000)
    struct.pack_into("<I", image, 0x421, IMAGE_BASE + 0x2050)

    # .rdata: export directory, import descriptors, scannable strings, and pointers.
    struct.pack_into(
        "<IIHHIIIIIII", image, 0x600,
        0, TIMESTAMP, 0, 0, 0x2040, 1, 2, 2, 0x2028, 0x2030, 0x2038,
    )
    struct.pack_into("<II", image, 0x628, 0x1000, 0x1020)
    struct.pack_into("<II", image, 0x630, 0x2050, 0x21FB)
    struct.pack_into("<HH", image, 0x638, 0, 1)
    image[0x640:0x64B] = b"sample.dll\0"
    image[0x650:0x656] = b"Alpha\0"
    struct.pack_into("<IIIII", image, 0x700, 0x2130, 0, 0, 0x2160, 0x2180)
    struct.pack_into("<III", image, 0x730, 0x2170, 0x80000007, 0)
    image[0x760:0x76A] = b"other.dll\0"
    struct.pack_into("<H", image, 0x770, 3)
    image[0x772:0x778] = b"Gamma\0"
    struct.pack_into("<III", image, 0x780, 0x2170, 0x80000007, 0)
    image[0x7A0:0x7AE] = b"ScannedString\0"
    image[0x7C0:0x7CA] = "Wide\0".encode("utf-16-le")
    struct.pack_into("<II", image, 0x7E0, IMAGE_BASE + 0x1000, IMAGE_BASE + 0x2000)
    image[0x7FB:0x800] = b"Beta\0"

    # .rsrc: type id 16 / named entry / language 0x409 reaching one data leaf.
    struct.pack_into("<IIHHHH", image, 0x800, 0, 0, 0, 0, 0, 1)
    struct.pack_into("<II", image, 0x810, 16, 0x80000018)
    struct.pack_into("<IIHHHH", image, 0x818, 0, 0, 0, 0, 1, 0)
    struct.pack_into("<II", image, 0x828, 0x80000060, 0x80000030)
    struct.pack_into("<IIHHHH", image, 0x830, 0, 0, 0, 0, 0, 1)
    struct.pack_into("<II", image, 0x840, 0x409, 0x48)
    struct.pack_into("<IIII", image, 0x848, 0x3070, 0x10, 1252, 0)
    struct.pack_into("<H", image, 0x860, 5)
    image[0x862:0x86C] = "Named".encode("utf-16-le")
    image[0x870:0x880] = b"ResourcePayload\0"

    # .reloc: two HIGHLOW fixups plus the ABSOLUTE padding a linker emits.
    struct.pack_into("<II", image, 0xA00, 0x1000, 16)
    struct.pack_into("<HHHH", image, 0xA08, (3 << 12) | 0x010, (3 << 12) | 0x021, 0, 0)

    # .data: one pointer in the file-backed part; the tail stays unwritten.
    struct.pack_into("<I", image, 0xC10, IMAGE_BASE + 0x2000)

    path.write_bytes(image)


class PeManifestTests(unittest.TestCase):
    def build(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.dll"
            make_pe32(path)
            return pe_manifest.build_manifest(path)

    def assertRejects(self, mutate, pattern):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.dll"
            make_pe32(path)
            image = bytearray(path.read_bytes())
            mutate(image)
            path.write_bytes(image)
            with self.assertRaisesRegex(ValueError, pattern):
                pe_manifest.build_manifest(path)

    def test_records_header_identity_with_integer_and_hexadecimal_forms(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.dll"
            make_pe32(path)
            manifest = pe_manifest.build_manifest(path)
            expected_sha256 = hashlib.sha256(path.read_bytes()).hexdigest()
        self.assertEqual(
            manifest["image"],
            {
                "name": "sample.dll",
                "size": 0xE00,
                "sha256": expected_sha256,
                "machine": 0x14C,
                "machine_hex": "0x014c",
                "magic": 0x10B,
                "magic_hex": "0x010b",
                "characteristics": 0x210E,
                "characteristics_hex": "0x0000210e",
                "timestamp": TIMESTAMP,
                "timestamp_hex": "0x41b0c4ad",
                "image_base": IMAGE_BASE,
                "image_base_hex": "0x10000000",
                "entry_point": 0x1000,
                "entry_point_hex": "0x00001000",
                "section_alignment": 0x1000,
                "file_alignment": 0x200,
                "size_of_image": 0x6000,
                "size_of_image_hex": "0x00006000",
            },
        )

    def test_records_every_data_directory_including_the_empty_ones(self):
        directories = self.build()["data_directories"]
        self.assertEqual(len(directories), 16)
        self.assertEqual(
            [(entry["name"], entry["rva_hex"], entry["size"]) for entry in directories if entry["size"]],
            [
                ("export", "0x00002000", 0x80),
                ("import", "0x00002100", 0x28),
                ("resource", "0x00003000", 0x60),
                ("basereloc", "0x00004000", 0x10),
                ("iat", "0x00002180", 0x0C),
            ],
        )

    def test_records_every_section_with_bounds_and_a_content_hash(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.dll"
            make_pe32(path)
            manifest = pe_manifest.build_manifest(path)
            raw = path.read_bytes()
        self.assertEqual(
            [(s["name"], s["rva"], s["virtual_size"], s["raw_offset"], s["raw_size"], s["executable"])
             for s in manifest["sections"]],
            [
                (".text", 0x1000, 0x40, 0x400, 0x200, True),
                (".rdata", 0x2000, 0x200, 0x600, 0x200, False),
                (".rsrc", 0x3000, 0x100, 0x800, 0x200, False),
                (".reloc", 0x4000, 0x20, 0xA00, 0x200, False),
                (".data", 0x5000, 0x300, 0xC00, 0x200, False),
            ],
        )
        self.assertEqual(manifest["sections"][0]["rva_hex"], "0x00001000")
        self.assertEqual(manifest["sections"][0]["characteristics_hex"], "0x60000020")
        self.assertEqual(manifest["sections"][0]["sha256"], hashlib.sha256(raw[0x400:0x600]).hexdigest())

    def test_extracts_executable_intervals_from_the_file_backed_extent(self):
        manifest = self.build()
        self.assertEqual(
            manifest["executable_intervals"],
            [
                {
                    "section": ".text",
                    "rva": 0x1000,
                    "rva_hex": "0x00001000",
                    "end_rva": 0x1040,
                    "end_rva_hex": "0x00001040",
                    "size": 0x40,
                    "raw_offset": 0x400,
                }
            ],
        )
        self.assertEqual(manifest["executable_bytes"], 0x40)

    def test_reads_named_exports_with_ordinals(self):
        exports = self.build()["exports"]
        self.assertEqual(exports["name"], "sample.dll")
        self.assertEqual(exports["ordinal_base"], 1)
        self.assertEqual(exports["named_count"], 2)
        self.assertEqual(
            exports["entries"],
            [
                {"ordinal": 1, "name": "Alpha", "rva": 0x1000, "rva_hex": "0x00001000", "forwarder": None},
                {"ordinal": 2, "name": "Beta", "rva": 0x1020, "rva_hex": "0x00001020", "forwarder": None},
            ],
        )

    def test_reads_imports_by_name_and_by_ordinal(self):
        self.assertEqual(
            self.build()["imports"],
            [
                {
                    "dll": "other.dll",
                    "name": "Gamma",
                    "ordinal": None,
                    "hint": 3,
                    "hint_name_rva": 0x2170,
                    "hint_name_rva_hex": "0x00002170",
                    "iat_rva": 0x2180,
                    "iat_rva_hex": "0x00002180",
                },
                {
                    "dll": "other.dll",
                    "name": None,
                    "ordinal": 7,
                    "hint": None,
                    "hint_name_rva": None,
                    "hint_name_rva_hex": None,
                    "iat_rva": 0x2184,
                    "iat_rva_hex": "0x00002184",
                },
            ],
        )

    def test_reads_base_relocations_in_file_order_including_absolute_padding(self):
        self.assertEqual(
            [(entry["rva_hex"], entry["type"], entry["type_name"], entry["block_rva_hex"])
             for entry in self.build()["relocations"]],
            [
                ("0x00001010", 3, "HIGHLOW", "0x00001000"),
                ("0x00001021", 3, "HIGHLOW", "0x00001000"),
                ("0x00001000", 0, "ABSOLUTE", "0x00001000"),
                ("0x00001000", 0, "ABSOLUTE", "0x00001000"),
            ],
        )

    def test_reads_the_resource_tree_down_to_its_data_leaves(self):
        self.assertEqual(
            self.build()["resources"],
            [
                {
                    "type": 16,
                    "name": "Named",
                    "language": 0x409,
                    "rva": 0x3070,
                    "rva_hex": "0x00003070",
                    "size": 0x10,
                    "codepage": 1252,
                }
            ],
        )

    def test_scans_strings_with_encoding_length_and_section(self):
        strings = self.build()["strings"]
        self.assertEqual(
            [(entry["rva_hex"], entry["encoding"], entry["value"]) for entry in strings],
            [
                ("0x00002040", "ascii", "sample.dll"),
                ("0x00002050", "ascii", "Alpha"),
                ("0x00002160", "ascii", "other.dll"),
                ("0x00002172", "ascii", "Gamma"),
                ("0x000021a0", "ascii", "ScannedString"),
                ("0x000021c0", "utf-16le", "Wide"),
                ("0x000021fb", "ascii", "Beta"),
            ],
        )
        self.assertEqual(
            strings[4],
            {
                "rva": 0x21A0,
                "rva_hex": "0x000021a0",
                "section": ".rdata",
                "encoding": "ascii",
                "length": 13,
                "size": 14,
                "value": "ScannedString",
            },
        )
        self.assertEqual(strings[5]["length"], 4)
        self.assertEqual(strings[5]["size"], 10)

    def test_string_scan_skips_the_resource_section(self):
        manifest = self.build()
        self.assertEqual(manifest["resources"][0]["rva"], 0x3070)
        self.assertEqual([entry for entry in manifest["strings"] if entry["section"] == ".rsrc"], [])
        self.assertNotIn("ResourcePayload", [entry["value"] for entry in manifest["strings"]])

    def test_scans_pointer_values_and_marks_relocated_sites(self):
        self.assertEqual(
            self.build()["pointers"],
            [
                {
                    "rva": 0x1010, "rva_hex": "0x00001010", "section": ".text",
                    "value": 0x10002000, "value_hex": "0x10002000",
                    "target_rva": 0x2000, "target_rva_hex": "0x00002000", "target_section": ".rdata",
                    "relocated": True,
                },
                {
                    "rva": 0x1021, "rva_hex": "0x00001021", "section": ".text",
                    "value": 0x10002050, "value_hex": "0x10002050",
                    "target_rva": 0x2050, "target_rva_hex": "0x00002050", "target_section": ".rdata",
                    "relocated": True,
                },
                {
                    "rva": 0x21E0, "rva_hex": "0x000021e0", "section": ".rdata",
                    "value": 0x10001000, "value_hex": "0x10001000",
                    "target_rva": 0x1000, "target_rva_hex": "0x00001000", "target_section": ".text",
                    "relocated": False,
                },
                {
                    "rva": 0x21E4, "rva_hex": "0x000021e4", "section": ".rdata",
                    "value": 0x10002000, "value_hex": "0x10002000",
                    "target_rva": 0x2000, "target_rva_hex": "0x00002000", "target_section": ".rdata",
                    "relocated": False,
                },
                {
                    "rva": 0x5010, "rva_hex": "0x00005010", "section": ".data",
                    "value": 0x10002000, "value_hex": "0x10002000",
                    "target_rva": 0x2000, "target_rva_hex": "0x00002000", "target_section": ".rdata",
                    "relocated": False,
                },
            ],
        )

    def test_records_the_string_scan_criterion_alongside_the_strings(self):
        scan = self.build()["string_scan"]
        self.assertEqual(scan["min_length"], 4)
        self.assertEqual(scan["encodings"], ["ascii", "utf-16le"])
        self.assertIn("NUL-terminated", scan["criterion"])
        self.assertIn("is not recorded", scan["criterion"])
        self.assertIn("0x20-0x7e", scan["criterion"])
        # The two utf-16le restrictions the scan enforces must be published too,
        # or a reader cannot tell absence-by-rule from absence-in-fact.
        self.assertIn("offsets even relative to the section start", scan["criterion"])
        self.assertIn("U+0020-U+007E", scan["criterion"])

    def test_rejects_an_rva_in_a_section_tail_that_no_file_bytes_back(self):
        self.assertRejects(
            lambda image: struct.pack_into(
                "<I", image, EXPORT_NAME_POINTERS + 4, UNBACKED_TAIL_RVA),
            "RVA 0x00005250 is not backed by file data",
        )

    def test_rejects_a_section_table_declared_past_the_end_of_the_file(self):
        self.assertRejects(
            lambda image: struct.pack_into("<H", image, OPTIONAL_SIZE, 0xE00),
            "truncated section header",
        )

    def test_rejects_a_non_ascii_export_name(self):
        self.assertRejects(
            lambda image: image.__setitem__(EXPORT_ALPHA_NAME + 1, 0xC3),
            "non-ASCII export name at RVA 0x00002050",
        )

    def test_rejects_a_relocation_site_outside_every_section(self):
        self.assertRejects(
            lambda image: struct.pack_into("<I", image, RELOCATION_BLOCK_PAGE, 0x9000),
            "relocation site RVA 0x00009010 does not map to a section",
        )

    def test_rejects_an_empty_base_relocation_block(self):
        self.assertRejects(
            lambda image: struct.pack_into("<I", image, RELOCATION_BLOCK_SIZE, 0),
            "empty base relocation block at RVA 0x00004000 abandons 16 bytes",
        )

    def test_rejects_a_file_without_the_pe_signature(self):
        self.assertRejects(
            lambda image: image.__setitem__(slice(PE_SIGNATURE, PE_SIGNATURE + 4), b"XX\0\0"),
            "not a PE image",
        )

    def test_rejects_a_non_pe32_optional_header(self):
        self.assertRejects(
            lambda image: struct.pack_into("<H", image, OPTIONAL_HEADER, 0x20B),
            "optional header is not PE32",
        )

    def test_rejects_an_optional_header_too_small_to_hold_its_fields(self):
        self.assertRejects(
            lambda image: struct.pack_into("<H", image, OPTIONAL_SIZE, 64),
            "truncated PE32 optional header",
        )

    def test_rejects_an_optional_header_too_small_for_its_data_directory(self):
        self.assertRejects(
            lambda image: struct.pack_into("<H", image, OPTIONAL_SIZE, 100),
            "truncated PE32 data directory",
        )

    def test_rejects_section_raw_data_running_past_the_end_of_the_file(self):
        self.assertRejects(
            lambda image: struct.pack_into("<I", image, RDATA_RAW_SIZE, 0x10000),
            r"section \.rdata raw data extends past the end of the file",
        )

    def test_rejects_an_export_name_without_a_terminator_in_its_section(self):
        self.assertRejects(
            lambda image: image.__setitem__(EXPORT_BETA_TERMINATOR, ord("X")),
            "unterminated export name at RVA 0x000021fb",
        )

    def test_rejects_an_export_name_rva_outside_every_section(self):
        self.assertRejects(
            lambda image: struct.pack_into("<I", image, EXPORT_NAME_POINTERS + 4, 0x9000),
            "RVA 0x00009000 does not map to a section",
        )

    def test_rejects_an_import_name_rva_outside_every_section(self):
        self.assertRejects(
            lambda image: struct.pack_into("<I", image, IMPORT_LOOKUP_TABLE, 0x9100),
            "RVA 0x00009100 does not map to a section",
        )

    def test_rejects_a_truncated_base_relocation_block(self):
        self.assertRejects(
            lambda image: struct.pack_into("<I", image, RELOCATION_BLOCK_SIZE, 4),
            "truncated base relocation block at RVA 0x00004000",
        )

    def test_rejects_a_resource_tree_deeper_than_three_levels(self):
        self.assertRejects(
            lambda image: struct.pack_into("<I", image, RESOURCE_LEAF_ENTRY, 0x80000048),
            "resource tree is deeper than three levels at RVA 0x00003048",
        )

    def test_rejects_resource_data_outside_every_section(self):
        self.assertRejects(
            lambda image: struct.pack_into("<I", image, RESOURCE_DATA_RVA, 0x9200),
            "RVA 0x00009200 does not map to a section",
        )


class PeManifestCliTests(unittest.TestCase):
    def test_writes_the_manifest_and_reports_its_measurements(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "sample.dll"
            make_pe32(source)
            output = root / "oracle" / "pe.json"
            result = subprocess.run(
                [
                    sys.executable,
                    str(TOOLS_DIR / "pe_manifest.py"),
                    "--input",
                    str(source),
                    "--output",
                    str(output),
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            manifest = json.loads(output.read_text(encoding="utf-8"))
        self.assertIn("pe=written", result.stdout)
        self.assertIn("machine=0x014c", result.stdout)
        self.assertIn("magic=0x010b", result.stdout)
        self.assertIn("exports=2", result.stdout)
        self.assertIn("executable_bytes=64", result.stdout)
        self.assertEqual(manifest["schema_version"], 1)
        self.assertEqual(manifest["exports"]["named_count"], 2)

    def test_unreadable_input_reports_cli_error_without_traceback(self):
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "missing.dll"
            result = subprocess.run(
                [
                    sys.executable,
                    str(TOOLS_DIR / "pe_manifest.py"),
                    "--input",
                    str(missing),
                    "--output",
                    str(Path(directory) / "pe.json"),
                ],
                capture_output=True,
                text=True,
                check=False,
            )
        self.assertEqual(result.returncode, 2)
        self.assertIn("error:", result.stderr)
        self.assertNotIn("Traceback", result.stderr)


if __name__ == "__main__":
    unittest.main()
