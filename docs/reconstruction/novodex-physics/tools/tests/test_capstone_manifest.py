import copy
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import capstone_manifest  # noqa: E402


IMAGE_BASE = 0x10000000
TEXT_RVA = 0x1000
TEXT_SIZE = 0x100
TEXT_RAW_OFFSET = 0x400
FILE_SIZE = 0x600
IAT_RVA = 0x2000
SWITCH_TABLE_RVA = 0x20C0

# Every instruction the fixture image contains, keyed by its offset in .text.
# Unwritten bytes stay 0xcc so the sweep has padding to propose alignment for.
CODE = {
    0x00: "55",                # push ebp                          <- entry point
    0x01: "8bec",              # mov ebp, esp
    0x03: "83ec10",            # sub esp, 0x10
    0x06: "e835000000",        # call 0x10001040
    0x0B: "d94508",            # fld dword ptr [ebp + 8]           x87
    0x0E: "0f28c1",            # movaps xmm0, xmm1                 SSE
    0x11: "ff1500200010",      # call dword ptr [0x10002000]       indirect
    0x17: "7403",              # je 0x1000101c
    0x19: "83c410",            # add esp, 0x10
    0x1C: "c9",                # leave                             <- branch target
    0x1D: "c3",                # ret
    0x20: "8b442404",          # mov eax, dword ptr [esp + 4]      <- export Alpha
    0x24: "ff2485c0200010",    # jmp dword ptr [eax*4 + 0x100020c0]
    0x30: "b801000000",        # mov eax, 1                        <- switch target
    0x35: "eb06",              # jmp 0x1000103d
    0x38: "b802000000",        # mov eax, 2                        <- switch target
    0x3D: "c3",                # ret                               <- jump target
    0x40: "8b4c2404",          # mov ecx, dword ptr [esp + 4]      <- call target
    0x44: "e917000000",        # jmp 0x10001060                    tail call
    0x50: "ff2500200010",      # jmp dword ptr [0x10002000]        import thunk
    0x60: "33c0",              # xor eax, eax                      <- Ghidra function
    0x62: "c20800",            # ret 8
    0x70: "90",                # nop                               <- relocation target
    0x71: "c3",                # ret
    0x78: "ffe0",              # jmp eax: no table and no thunk    <- Ghidra function
    0x80: "6870100010",        # push 0x10001070                   <- Ghidra function
    0x85: "58",                # pop eax
    0x86: "c3",                # ret
    0x87: "33c0c3",            # unreached but decodable: the sweep proposes code
    0x90: "33c9",              # xor ecx, ecx                      <- Ghidra function
    0x92: "0f0a",              # no such opcode: the descent stops here
    0x98: "ff2485a0100010",    # jmp dword ptr [eax*4 + 0x100010a0] <- Ghidra function
    0xA0: "b0100010b8100010",  # a jump table in .text: never code
    0xB0: "33c0c3",            # xor eax, eax; ret                 <- switch target
    0xB8: "6a0158c3",          # push 1; pop eax; ret              <- switch target
    0xC0: "0f0a",              # no such opcode in a gap: the sweep proposes data
}

# Offsets the alignment proposal must cover, and the gaps that differ.
ALIGNMENT_GAPS = ((0x1E, 0x20), (0x2B, 0x30), (0x37, 0x38), (0x3E, 0x40),
                  (0x49, 0x50), (0x56, 0x60), (0x65, 0x70), (0x72, 0x78),
                  (0x7A, 0x80), (0xB3, 0xB8))
CODE_GAP = (0x87, 0x90)
JUMP_TABLE_GAP = (0x9F, 0xB0)
DATA_GAP = (0xBC, 0x100)


def make_binary(path):
    """Write a file whose .text holds the fixture instructions and 0xcc padding."""
    image = bytearray(b"\0" * FILE_SIZE)
    image[TEXT_RAW_OFFSET:TEXT_RAW_OFFSET + TEXT_SIZE] = b"\xcc" * TEXT_SIZE
    for offset, encoded in CODE.items():
        raw = bytes.fromhex(encoded)
        image[TEXT_RAW_OFFSET + offset:TEXT_RAW_OFFSET + offset + len(raw)] = raw
    path.write_bytes(image)
    return bytes(image)


def pointer(rva, section, target_rva, target_section, relocated):
    return {
        "rva": rva, "rva_hex": f"0x{rva:08x}", "section": section,
        "value": IMAGE_BASE + target_rva, "value_hex": f"0x{IMAGE_BASE + target_rva:08x}",
        "target_rva": target_rva, "target_rva_hex": f"0x{target_rva:08x}",
        "target_section": target_section, "relocated": relocated,
    }


def make_pe(data):
    """Build the PE oracle the fixture image would produce."""
    return {
        "schema_version": capstone_manifest.SCHEMA_VERSION,
        "image": {
            "name": "sample.dll", "size": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
            "image_base": IMAGE_BASE, "image_base_hex": "0x10000000",
            "entry_point": TEXT_RVA, "entry_point_hex": "0x00001000",
            "size_of_image": 0x6000,
        },
        "sections": [
            {"name": ".text", "rva": TEXT_RVA, "virtual_size": TEXT_SIZE,
             "raw_offset": TEXT_RAW_OFFSET, "raw_size": 0x200, "executable": True},
            {"name": ".rdata", "rva": 0x2000, "virtual_size": 0x200,
             "raw_offset": 0x600, "raw_size": 0x200, "executable": False},
            {"name": ".data", "rva": 0x5000, "virtual_size": 0x200,
             "raw_offset": 0x800, "raw_size": 0x200, "executable": False},
        ],
        "executable_intervals": [{
            "section": ".text", "rva": TEXT_RVA, "rva_hex": "0x00001000",
            "end_rva": TEXT_RVA + TEXT_SIZE, "end_rva_hex": "0x00001100",
            "size": TEXT_SIZE, "raw_offset": TEXT_RAW_OFFSET,
        }],
        "executable_bytes": TEXT_SIZE,
        "exports": {
            "name": "sample.dll", "ordinal_base": 1, "named_count": 1,
            "entries": [{"ordinal": 1, "name": "Alpha", "rva": 0x1020,
                         "rva_hex": "0x00001020", "forwarder": None}],
        },
        "imports": [{"dll": "other.dll", "name": "Gamma", "ordinal": None, "hint": 3,
                     "iat_rva": IAT_RVA, "iat_rva_hex": "0x00002000"}],
        "pointers": [
            # An immediate of `push 0x10001070`: the program uses it as a value.
            pointer(0x1081, ".text", 0x1070, ".text", True),
            # The displacement of the jump-table read: decoding it would fabricate
            # instructions out of the table.
            pointer(0x109B, ".text", 0x10A0, ".text", True),
            # The table's own slots, which no decoded instruction covers.
            pointer(0x10A0, ".text", 0x10B0, ".text", True),
            pointer(0x10A4, ".text", 0x10B8, ".text", True),
            # Two jump-table slots, then a slot that leaves the executable extent.
            pointer(SWITCH_TABLE_RVA, ".rdata", 0x1030, ".text", True),
            pointer(SWITCH_TABLE_RVA + 4, ".rdata", 0x1038, ".text", True),
            pointer(SWITCH_TABLE_RVA + 8, ".rdata", 0x4000, ".rdata", True),
            # A function-pointer table slot naming the import thunk.
            pointer(0x20D0, ".rdata", 0x1050, ".text", True),
            # .text by max(virtual_size, raw_size) but past the executable extent.
            pointer(0x20D4, ".rdata", 0x1100, ".text", True),
            # An unrelocated coincidence: a rebasable image cannot use it.
            pointer(0x20D8, ".rdata", 0x1000, ".text", False),
            # A relocated pointer that does not name code at all.
            pointer(0x20DC, ".data", 0x5010, ".data", True),
        ],
    }


def make_ghidra(data, pin):
    """Build the Ghidra oracle the fixture image would produce."""
    return {
        "schema_version": capstone_manifest.SCHEMA_VERSION,
        "ghidra": {
            "version": pin["ghidra"]["version"],
            "metadata": {"Executable SHA256": hashlib.sha256(data).hexdigest(),
                         "SectionAlignment": "4096"},
        },
        "functions": [
            {"rva": "0x00001040", "name": "thunk_FUN_10001060", "thunk": True,
             "thunk_target": {"rva": "0x00001060", "space": "ram", "space_offset": None}},
            {"rva": "0x00001050", "name": "Gamma", "thunk": True,
             "thunk_target": {"rva": None, "space": "EXTERNAL", "space_offset": 4}},
            {"rva": "0x00001060", "name": "FUN_10001060", "thunk": False, "thunk_target": None},
            {"rva": "0x00001078", "name": "FUN_10001078", "thunk": False, "thunk_target": None},
            {"rva": "0x00001080", "name": "FUN_10001080", "thunk": False, "thunk_target": None},
            {"rva": "0x00001090", "name": "FUN_10001090", "thunk": False, "thunk_target": None},
            {"rva": "0x00001098", "name": "FUN_10001098", "thunk": False, "thunk_target": None},
        ],
    }


class _Fixture(unittest.TestCase):
    """A built manifest plus the helpers every test group needs."""

    @classmethod
    def setUpClass(cls):
        with tempfile.TemporaryDirectory() as directory:
            cls.data = make_binary(Path(directory) / "sample.dll")
        cls.pin = capstone_manifest.load_pin()
        cls.pe = make_pe(cls.data)
        cls.ghidra = make_ghidra(cls.data, cls.pin)
        cls.manifest = capstone_manifest.build_manifest(
            cls.data, cls.pe, cls.ghidra, pin=cls.pin)

    def instruction(self, offset):
        rva = f"0x{TEXT_RVA + offset:08x}"
        found = [entry for entry in self.manifest["instructions"] if entry["rva"] == rva]
        self.assertEqual(len(found), 1, f"no instruction at {rva}")
        return found[0]

    def entry(self, offset):
        rva = f"0x{TEXT_RVA + offset:08x}"
        found = [entry for entry in self.manifest["entries"] if entry["rva"] == rva]
        self.assertEqual(len(found), 1, f"no entry at {rva}")
        return found[0]

    def chunk(self, offset):
        rva = f"0x{TEXT_RVA + offset:08x}"
        found = [entry for entry in self.manifest["chunks"] if entry["rva"] == rva]
        self.assertEqual(len(found), 1, f"no chunk at {rva}")
        return found[0]

    def gap(self, offset):
        rva = f"0x{TEXT_RVA + offset:08x}"
        found = [entry for entry in self.manifest["coverage_gaps"] if entry["rva"] == rva]
        self.assertEqual(len(found), 1, f"no coverage gap at {rva}")
        return found[0]


class InstructionRecordTests(_Fixture):
    def test_records_a_relative_call_with_its_bytes_text_and_target(self):
        self.assertEqual(
            self.instruction(0x06),
            {"rva": "0x00001006", "size": 5, "bytes": "e835000000", "mnemonic": "call",
             "operands": "0x10001040", "flow": "call", "indirect": False,
             "target_rva": "0x00001040", "stack_delta": -4},
        )

    def test_records_a_relative_jump_as_an_unconditional_flow_break(self):
        self.assertEqual(
            self.instruction(0x35),
            {"rva": "0x00001035", "size": 2, "bytes": "eb06", "mnemonic": "jmp",
             "operands": "0x1000103d", "flow": "jump", "indirect": False,
             "target_rva": "0x0000103d", "stack_delta": 0},
        )

    def test_records_a_conditional_branch_separately_from_an_unconditional_jump(self):
        self.assertEqual(
            self.instruction(0x17),
            {"rva": "0x00001017", "size": 2, "bytes": "7403", "mnemonic": "je",
             "operands": "0x1000101c", "flow": "branch", "indirect": False,
             "target_rva": "0x0000101c", "stack_delta": 0},
        )

    def test_records_an_indirect_call_without_inventing_a_target(self):
        self.assertEqual(
            self.instruction(0x11),
            {"rva": "0x00001011", "size": 6, "bytes": "ff1500200010", "mnemonic": "call",
             "operands": "dword ptr [0x10002000]", "flow": "call", "indirect": True,
             "target_rva": None, "stack_delta": -4},
        )

    def test_records_an_x87_instruction_without_touching_the_stack_pointer(self):
        self.assertEqual(
            self.instruction(0x0B),
            {"rva": "0x0000100b", "size": 3, "bytes": "d94508", "mnemonic": "fld",
             "operands": "dword ptr [ebp + 8]", "flow": "sequential", "indirect": False,
             "target_rva": None, "stack_delta": 0},
        )

    def test_records_an_sse_instruction_with_its_register_operands(self):
        self.assertEqual(
            self.instruction(0x0E),
            {"rva": "0x0000100e", "size": 3, "bytes": "0f28c1", "mnemonic": "movaps",
             "operands": "xmm0, xmm1", "flow": "sequential", "indirect": False,
             "target_rva": None, "stack_delta": 0},
        )

    def test_measures_the_stack_delta_of_every_instruction_that_moves_esp(self):
        self.assertEqual(
            [(self.instruction(offset)["mnemonic"], self.instruction(offset)["stack_delta"])
             for offset in (0x00, 0x03, 0x19, 0x1C, 0x62, 0x85)],
            [("push", -4), ("sub", -16), ("add", 16), ("leave", None), ("ret", 12), ("pop", 4)],
        )

    def test_records_an_indirect_jump_through_a_jump_table(self):
        self.assertEqual(
            self.instruction(0x24),
            {"rva": "0x00001024", "size": 7, "bytes": "ff2485c0200010", "mnemonic": "jmp",
             "operands": "dword ptr [eax*4 + 0x100020c0]", "flow": "jump", "indirect": True,
             "target_rva": None, "stack_delta": 0},
        )


class SeedingTests(_Fixture):
    def test_seeds_the_entry_point_the_exports_and_the_ghidra_functions(self):
        self.assertEqual(self.entry(0x00)["sources"], ["entry_point"])
        self.assertEqual(self.entry(0x20)["sources"], ["export"])
        self.assertEqual(self.entry(0x90)["sources"], ["ghidra_function"])

    def test_seeds_a_code_embedded_absolute_address_from_its_relocation(self):
        self.assertEqual(self.entry(0x70)["sources"], ["relocation_target"])
        self.assertEqual(self.entry(0x70)["kind"], "function")

    def test_tells_a_code_pointer_apart_from_a_table_the_code_reads(self):
        # Only the immediate names code. The displacement names the jump table and
        # the two slots inside it are covered by no instruction at all, so neither
        # may be decoded - doing so fabricates instructions out of the table.
        self.assertEqual(
            self.manifest["relocation_uses"],
            [{"rva": "0x00001081", "target_rva": "0x00001070", "use": "immediate"},
             {"rva": "0x0000109b", "target_rva": "0x000010a0", "use": "displacement"},
             {"rva": "0x000010a0", "target_rva": "0x000010b0", "use": "unresolved"},
             {"rva": "0x000010a4", "target_rva": "0x000010b8", "use": "unresolved"}],
        )
        self.assertEqual(
            [entry["rva"] for entry in self.manifest["entries"]
             if "relocation_target" in entry["sources"]],
            ["0x00001070"],
        )
        self.assertNotIn("0x000010a0", [entry["rva"] for entry in self.manifest["entries"]])

    def test_seeds_a_function_pointer_table_slot_outside_the_executable_section(self):
        self.assertEqual(
            self.entry(0x50)["sources"], ["ghidra_function", "pointer_table"])

    def test_ignores_a_pointer_no_relocation_fixes_up(self):
        # 0x00001000 is named by an unrelocated pointer record and by the entry
        # point; only the entry point may account for it.
        self.assertEqual(self.entry(0x00)["sources"], ["entry_point"])

    def test_seeds_the_target_of_a_ghidra_thunk_and_tolerates_an_external_one(self):
        self.assertIn("thunk_target", self.entry(0x60)["sources"])
        self.assertEqual(
            [entry["rva"] for entry in self.manifest["entries"]
             if "thunk_target" in entry["sources"]],
            ["0x00001060"],
        )

    def test_seeds_direct_call_jump_and_branch_targets_found_while_decoding(self):
        self.assertIn("direct_call", self.entry(0x40)["sources"])
        self.assertIn("direct_jump", self.entry(0x3D)["sources"])
        self.assertIn("direct_branch", self.entry(0x1C)["sources"])

    def test_recovers_a_tail_call_to_an_independently_known_function(self):
        self.assertEqual(
            self.manifest["tail_calls"],
            [{"from_rva": "0x00001044", "to_rva": "0x00001060"}],
        )
        self.assertIn("tail_call", self.entry(0x60)["sources"])

    def test_counts_every_entry_under_each_source_that_names_it(self):
        # pointer_table names four entries: two jump-table slots, the import
        # thunk, and the one slot that leaves the executable extent.
        # pointer_table names four targets, but one of them lies outside the
        # executable extent; the census counts the same entries counts.entries
        # does, so that one is reported by seeds_outside_executable instead.
        self.assertEqual(
            self.manifest["seed_sources"],
            {"entry_point": 1, "export": 1, "ghidra_function": 7, "thunk_target": 1,
             "relocation_target": 1, "pointer_table": 3, "switch_target": 4,
             "direct_call": 1, "direct_jump": 2, "direct_branch": 1, "tail_call": 1},
        )
        self.assertEqual(
            sum(self.manifest["seed_sources"].values()),
            sum(len(entry["sources"]) for entry in self.manifest["entries"]),
        )

    def test_records_a_seed_the_executable_extent_does_not_contain(self):
        self.assertEqual(
            self.manifest["seeds_outside_executable"],
            [{"rva": "0x00001100", "sources": ["pointer_table"]}],
        )


class SwitchTableTests(_Fixture):
    def test_walks_a_jump_table_until_a_slot_stops_naming_executable_code(self):
        self.assertEqual(
            self.manifest["switch_tables"],
            [{"jump_rva": "0x00001024", "table_rva": "0x000020c0", "entries": 2,
              "targets": ["0x00001030", "0x00001038"]},
             {"jump_rva": "0x00001098", "table_rva": "0x000010a0", "entries": 2,
              "targets": ["0x000010b0", "0x000010b8"]}],
        )

    def test_a_switch_target_stays_an_intra_function_block(self):
        # A jump-table slot is also a relocated pointer, so the classifier has to
        # prefer the switch evidence or it would call a block a function.
        self.assertEqual(self.entry(0x30)["sources"], ["pointer_table", "switch_target"])
        self.assertEqual(self.entry(0x30)["kind"], "block")


class ImportThunkTests(_Fixture):
    def test_names_the_import_an_indirect_jump_through_the_iat_reaches(self):
        self.assertEqual(
            self.manifest["import_thunks"],
            [{"rva": "0x00001050", "iat_rva": "0x00002000", "dll": "other.dll",
              "name": "Gamma", "ordinal": None}],
        )


class ChunkTests(_Fixture):
    def test_cuts_a_chunk_where_a_branch_target_splits_straight_line_code(self):
        self.assertEqual(
            self.chunk(0x00),
            {"rva": "0x00001000", "end_rva": "0x0000101c", "size": 28,
             "instructions": 9, "terminator": "fallthrough"},
        )
        self.assertEqual(
            self.chunk(0x1C),
            {"rva": "0x0000101c", "end_rva": "0x0000101e", "size": 2,
             "instructions": 2, "terminator": "return"},
        )

    def test_ends_a_chunk_at_the_byte_the_decoder_refuses(self):
        self.assertEqual(
            self.chunk(0x90),
            {"rva": "0x00001090", "end_rva": "0x00001092", "size": 2,
             "instructions": 1, "terminator": "undecodable"},
        )

    def test_classifies_every_entry_as_a_function_or_an_intra_function_block(self):
        self.assertEqual(
            {entry["rva"]: entry["kind"] for entry in self.manifest["entries"]},
            {"0x00001000": "function", "0x0000101c": "block", "0x00001020": "function",
             "0x00001030": "block", "0x00001038": "block", "0x0000103d": "block",
             "0x00001040": "function", "0x00001050": "function", "0x00001060": "function",
             "0x00001070": "function", "0x00001078": "function", "0x00001080": "function",
             "0x00001090": "function", "0x00001098": "function", "0x000010b0": "block",
             "0x000010b8": "block"},
        )

    def test_the_chunks_partition_the_decoded_instruction_bytes(self):
        self.assertEqual(
            sum(chunk["size"] for chunk in self.manifest["chunks"]),
            self.manifest["coverage"]["instruction_bytes"],
        )
        self.assertEqual(self.manifest["counts"]["chunks_without_entry"], 0)
        self.assertEqual(self.manifest["overlaps"], [])


class CoverageTests(_Fixture):
    def test_records_the_byte_the_decoder_refuses_rather_than_skipping_it(self):
        self.assertEqual(
            self.manifest["undecodable"],
            [{"rva": "0x00001092", "byte": "0x0f", "chunk_rva": "0x00001090"}],
        )

    def test_every_executable_byte_is_instruction_undecodable_or_unclassified(self):
        coverage = self.manifest["coverage"]
        self.assertEqual(coverage["executable_bytes"], TEXT_SIZE)
        self.assertEqual(coverage["instruction_bytes"], 101)
        self.assertEqual(coverage["undecodable_bytes"], 1)
        self.assertEqual(coverage["unclassified_bytes"], 154)
        self.assertEqual(
            coverage["instruction_bytes"] + coverage["undecodable_bytes"]
            + coverage["unclassified_bytes"],
            coverage["executable_bytes"],
        )

    def test_the_sweep_proposes_alignment_for_a_run_of_padding(self):
        self.assertEqual(
            [(gap["rva"], gap["end_rva"]) for gap in self.manifest["coverage_gaps"]
             if gap["proposal"] == "alignment"],
            [(f"0x{TEXT_RVA + start:08x}", f"0x{TEXT_RVA + end:08x}")
             for start, end in ALIGNMENT_GAPS],
        )
        self.assertEqual(self.gap(0x37), {
            "rva": "0x00001037", "end_rva": "0x00001038", "size": 1, "bytes": "cc",
            "proposal": "alignment", "swept_instructions": 1, "undecodable_offsets": []})

    def test_the_sweep_proposes_code_for_a_gap_that_disassembles_cleanly(self):
        gap = self.gap(CODE_GAP[0])
        self.assertEqual(gap["proposal"], "code")
        self.assertEqual(gap["bytes"], "33c0c3" + "cc" * 6)
        self.assertEqual(gap["undecodable_offsets"], [])

    def test_the_sweep_proposes_data_when_it_cannot_decode_the_whole_gap(self):
        gap = self.gap(DATA_GAP[0])
        self.assertEqual(gap["proposal"], "data")
        self.assertEqual(gap["size"], DATA_GAP[1] - DATA_GAP[0])
        self.assertEqual(gap["undecodable_offsets"], [0xC0 - DATA_GAP[0]])

    def test_the_proposals_partition_the_unclassified_bytes(self):
        coverage = self.manifest["coverage"]
        self.assertEqual(
            coverage["proposed_alignment_bytes"] + coverage["proposed_code_bytes"]
            + coverage["proposed_data_bytes"],
            coverage["unclassified_bytes"],
        )
        self.assertEqual(coverage["proposed_alignment_bytes"], 55)
        self.assertEqual(coverage["proposed_code_bytes"], 31)
        self.assertEqual(coverage["proposed_data_bytes"], 68)

    def test_counts_the_bytes_the_sweep_refused_apart_from_the_descents(self):
        # Two different passes refuse bytes and only the descent's are a byte
        # state; a reader of `undecodable_bytes` alone would conclude that
        # nothing in the extent resists Capstone, which is not what happened.
        self.assertEqual(self.manifest["coverage"]["undecodable_bytes"], 1)
        self.assertEqual(self.manifest["coverage"]["sweep_undecodable_bytes"], 1)
        self.assertEqual(self.manifest["counts"]["descent_undecodable"], 1)
        self.assertEqual(self.manifest["counts"]["sweep_undecodable"], 1)
        self.assertEqual(
            self.manifest["counts"]["sweep_undecodable"],
            sum(len(gap["undecodable_offsets"]) for gap in self.manifest["coverage_gaps"]),
        )

    def test_counts_the_indirect_jumps_nothing_in_the_evidence_resolves(self):
        # `jmp eax` names no table and no import, so it appears in no supporting
        # table; the count is what keeps that absence in the evidence.
        self.assertEqual(self.manifest["counts"]["unresolved_indirect_jumps"], 1)
        self.assertEqual(self.instruction(0x78)["mnemonic"], "jmp")
        self.assertEqual(self.instruction(0x78)["indirect"], True)

    def test_a_jump_table_the_descent_refused_to_decode_stays_unclassified(self):
        # The sweep will happily read the table as instructions, which is exactly
        # why its verdict is a proposal and the switch table is the proof.
        gap = self.gap(JUMP_TABLE_GAP[0])
        self.assertEqual(gap["size"], JUMP_TABLE_GAP[1] - JUMP_TABLE_GAP[0])
        self.assertIn("b0100010b8100010", gap["bytes"])
        self.assertEqual(gap["proposal"], "code")

    def test_the_recursive_descent_runs_to_a_fixed_point(self):
        discovery = self.manifest["discovery"]
        # The first round decodes what the oracles name and what its own control
        # flow reaches; the second adds the one address an immediate proved to be
        # code, and then nothing is left for either loop to find.
        self.assertEqual(
            [(step["round"], step["new_entries"]) for step in discovery["iterations"]],
            [(1, 12), (1, 4), (1, 0), (2, 1), (2, 0)],
        )
        self.assertEqual(discovery["rounds"],
                         [{"round": 1, "relocation_seeds": 1},
                          {"round": 2, "relocation_seeds": 0}])
        # Both loops exit only when they find nothing, so neither terminal count
        # above could have been anything else. What is worth publishing is how many
        # seed claims a fresh pass re-derived and found already recorded: 14 from
        # the oracles, 1 from an immediate, 4 direct edges and 4 jump-table slots.
        self.assertNotIn("fixed_point", discovery)
        self.assertEqual(discovery["rechecked_seeds"], 23)


class ManifestShapeTests(_Fixture):
    def test_records_the_capstone_configuration_the_corpus_was_produced_under(self):
        self.assertEqual(
            self.manifest["capstone"],
            {"version": capstone_manifest.capstone.__version__, "arch": "x86",
             "mode": "32", "syntax": "intel", "detail": True},
        )
        self.assertEqual(self.manifest["schema_version"], capstone_manifest.SCHEMA_VERSION)

    def test_publishes_the_criterion_behind_every_judgement_it_makes(self):
        method = self.manifest["method"]
        self.assertEqual(sorted(method), ["chunk_kind", "coverage", "descent",
                                          "relocation_use", "seeding", "stack_delta",
                                          "sweep", "switch_table_candidate", "tail_call"])
        self.assertIn("proposal", method["sweep"])
        self.assertIn("scale 4", method["switch_table_candidate"])
        self.assertIn("no relocation fixes up", method["seeding"])
        self.assertIn("displacement of a memory operand", method["relocation_use"])
        # The three residuals a reader would otherwise have to already know about.
        self.assertIn("a jump table listed in switch_tables", method["sweep"])
        self.assertIn("labels entries rather than contributing them", method["tail_call"])
        self.assertIn("sweep_undecodable_bytes", method["coverage"])

    def test_reports_the_executable_extent_it_was_asked_to_account_for(self):
        self.assertEqual(
            self.manifest["executable_intervals"],
            [{"section": ".text", "rva": "0x00001000", "end_rva": "0x00001100",
              "size": TEXT_SIZE}],
        )

    def test_counts_every_table_the_manifest_carries(self):
        self.assertEqual(
            self.manifest["counts"],
            {"entries": 16, "chunks": 16, "instructions": 35,
             "descent_undecodable": 1, "sweep_undecodable": 1, "switch_tables": 2,
             "import_thunks": 1, "tail_calls": 1, "unresolved_indirect_jumps": 1,
             "relocation_uses": 4, "coverage_gaps": 14, "overlaps": 0,
             "seeds_outside_executable": 1, "chunks_without_entry": 0},
        )


class PinTests(unittest.TestCase):
    def setUp(self):
        self.pin = capstone_manifest.load_pin()

    def rejects(self, mutate, pattern):
        pin = copy.deepcopy(self.pin)
        mutate(pin)
        with self.assertRaisesRegex(ValueError, pattern):
            capstone_manifest.check_against_pin(pin)

    def test_accepts_the_recorded_pin(self):
        self.assertIsNone(capstone_manifest.check_against_pin(self.pin))

    def test_rejects_an_unreadable_toolchain_pin(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, "cannot read the toolchain pin"):
                capstone_manifest.load_pin(Path(directory) / "missing.json")

    def test_rejects_a_pin_written_against_another_schema(self):
        self.rejects(
            lambda pin: pin.__setitem__("schema_version", 2),
            "toolchain pin schema version is 2 but this tool reads 1",
        )

    def test_rejects_a_capstone_version_the_pin_does_not_record(self):
        self.rejects(
            lambda pin: pin["capstone"].__setitem__("version", "4.0.2"),
            "capstone.version is '.*' but the pin records '4.0.2'",
        )

    def test_rejects_a_capstone_package_whose_content_moved(self):
        self.rejects(
            lambda pin: pin["capstone"].__setitem__("package_sha256", "0" * 64),
            f"capstone.package_sha256 is '.*' but the pin records '{'0' * 64}'",
        )

    def test_rejects_a_pin_whose_analysis_options_hash_was_edited(self):
        self.rejects(
            lambda pin: pin.__setitem__("analysis_options_sha256", "1" * 64),
            f"analysis-options hash is .* but the pin records '{'1' * 64}'",
        )


class InputRejectionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with tempfile.TemporaryDirectory() as directory:
            cls.data = make_binary(Path(directory) / "sample.dll")
        cls.pin = capstone_manifest.load_pin()

    def rejects(self, mutate, pattern):
        pe = make_pe(self.data)
        ghidra = make_ghidra(self.data, self.pin)
        mutate(pe, ghidra)
        with self.assertRaisesRegex(ValueError, pattern):
            capstone_manifest.build_manifest(self.data, pe, ghidra, pin=self.pin)

    def test_rejects_a_pe_oracle_written_against_another_schema(self):
        self.rejects(
            lambda pe, ghidra: pe.__setitem__("schema_version", 7),
            "the PE oracle records schema version 7 but this tool reads 1",
        )

    def test_rejects_a_ghidra_oracle_written_against_another_schema(self):
        self.rejects(
            lambda pe, ghidra: ghidra.__setitem__("schema_version", 9),
            "the Ghidra oracle records schema version 9 but this tool reads 1",
        )

    def test_rejects_a_pe_oracle_describing_another_image(self):
        self.rejects(
            lambda pe, ghidra: pe["image"].__setitem__("sha256", "a" * 64),
            f"the PE oracle describes image {'a' * 64} but the binary is",
        )

    def test_rejects_a_ghidra_oracle_describing_another_image(self):
        self.rejects(
            lambda pe, ghidra: ghidra["ghidra"]["metadata"].__setitem__(
                "Executable SHA256", "b" * 64),
            f"the Ghidra oracle describes image {'b' * 64} but the binary is",
        )

    def test_rejects_a_ghidra_oracle_from_an_unpinned_ghidra(self):
        self.rejects(
            lambda pe, ghidra: ghidra["ghidra"].__setitem__("version", "11.4.2"),
            "ghidra.version is '11.4.2' but the pin records",
        )

    def test_rejects_a_pe_oracle_with_nothing_executable_to_account_for(self):
        self.rejects(
            lambda pe, ghidra: pe.__setitem__("executable_intervals", []),
            "the PE oracle records no executable interval",
        )

    def test_rejects_an_executable_interval_the_file_does_not_back(self):
        self.rejects(
            lambda pe, ghidra: pe["executable_intervals"][0].__setitem__(
                "raw_offset", FILE_SIZE),
            "executable interval 0x00001000 is not backed by the file",
        )

    def test_rejects_a_coverage_split_that_loses_a_byte(self):
        with self.assertRaisesRegex(
                ValueError, "coverage accounts for 99 of 100 executable bytes"):
            capstone_manifest.check_coverage(100, 60, 9, 30)

    def test_rejects_a_descent_that_recorded_an_entry_it_never_decoded(self):
        entries = {0x1000: {"entry_point"}, 0x1040: {"direct_call"}}
        seeds = [("entry_point", 0x1000), ("direct_call", 0x1040)]
        with self.assertRaisesRegex(
                ValueError,
                "the descent recorded 1 entries it never decoded, the first "
                "being 0x00001040"):
            capstone_manifest.check_fixed_point(entries, seeds, entries, {0x1000: {}})

    def test_rejects_a_descent_a_fresh_pass_would_still_add_to(self):
        # A target the descent reached but never recorded, and one it recorded
        # under a different source: neither may pass the gate.
        entries = {0x1000: {"entry_point"}, 0x1040: {"direct_call"}}
        seeds = [("entry_point", 0x1000), ("direct_call", 0x1040),
                 ("direct_branch", 0x1040), ("switch_target", 0x1080)]
        with self.assertRaisesRegex(
                ValueError,
                "the descent is not at a fixed point: 2 seeds are unrecorded, "
                "the first being direct_branch at 0x00001040"):
            capstone_manifest.check_fixed_point(entries, seeds, entries, entries)


class CapstoneManifestCliTests(unittest.TestCase):
    def run_cli(self, directory, **overrides):
        root = Path(directory)
        binary = root / "sample.dll"
        data = make_binary(binary)
        pin = capstone_manifest.load_pin()
        (root / "pe.json").write_text(json.dumps(make_pe(data)), encoding="utf-8")
        (root / "ghidra.json").write_text(
            json.dumps(make_ghidra(data, pin)), encoding="utf-8")
        arguments = {"--binary": str(binary), "--pe": str(root / "pe.json"),
                     "--ghidra": str(root / "ghidra.json"),
                     "--output": str(root / "oracle" / "capstone" / "manifest.json")}
        arguments.update(overrides)
        command = [sys.executable, str(TOOLS_DIR / "capstone_manifest.py")]
        for flag, value in arguments.items():
            command += [flag, value]
        return subprocess.run(command, capture_output=True, text=True, check=False), arguments

    def test_writes_the_corpus_and_reports_its_measurements(self):
        with tempfile.TemporaryDirectory() as directory:
            result, arguments = self.run_cli(directory)
            self.assertEqual(result.returncode, 0, result.stderr)
            manifest = json.loads(Path(arguments["--output"]).read_text(encoding="utf-8"))
        self.assertIn("capstone=written", result.stdout)
        self.assertIn("instructions=35", result.stdout)
        self.assertIn("instruction_bytes=101", result.stdout)
        self.assertIn("undecodable_bytes=1", result.stdout)
        self.assertIn("sweep_undecodable_bytes=1", result.stdout)
        self.assertIn("unclassified_bytes=154", result.stdout)
        self.assertIn("executable_bytes=256", result.stdout)
        self.assertIn("rounds=2", result.stdout)
        self.assertIn("iterations=5", result.stdout)
        self.assertIn("rechecked_seeds=23", result.stdout)
        self.assertEqual(manifest["counts"]["entries"], 16)

    def test_unreadable_input_reports_cli_error_without_traceback(self):
        with tempfile.TemporaryDirectory() as directory:
            result, _ = self.run_cli(
                directory, **{"--binary": str(Path(directory) / "missing.dll")})
        self.assertEqual(result.returncode, 2)
        self.assertIn("error:", result.stderr)
        self.assertNotIn("Traceback", result.stderr)


# Every message the tool can raise, against the test that drives it. Two sites
# are checked once per oracle, so eleven sites need fourteen negative tests; a
# new raise site fails this census until it is added here with a test to match.
RAISE_SITES = {
    "analysis-options hash is {} but the pin records {}":
        "test_rejects_a_pin_whose_analysis_options_hash_was_edited",
    "cannot read the toolchain pin: {}":
        "test_rejects_an_unreadable_toolchain_pin",
    "capstone.{} is {} but the pin records {}":
        "test_rejects_a_capstone_version_the_pin_does_not_record, "
        "test_rejects_a_capstone_package_whose_content_moved",
    "coverage accounts for {} of {} executable bytes":
        "test_rejects_a_coverage_split_that_loses_a_byte",
    "the descent is not at a fixed point: {} seeds are unrecorded, the first being {} at {}":
        "test_rejects_a_descent_a_fresh_pass_would_still_add_to",
    "the descent recorded {} entries it never decoded, the first being {}":
        "test_rejects_a_descent_that_recorded_an_entry_it_never_decoded",
    "executable interval {} is not backed by the file":
        "test_rejects_an_executable_interval_the_file_does_not_back",
    "ghidra.version is {} but the pin records {}":
        "test_rejects_a_ghidra_oracle_from_an_unpinned_ghidra",
    "the PE oracle records no executable interval":
        "test_rejects_a_pe_oracle_with_nothing_executable_to_account_for",
    "the {} oracle describes image {} but the binary is {}":
        "test_rejects_a_pe_oracle_describing_another_image, "
        "test_rejects_a_ghidra_oracle_describing_another_image",
    "the {} oracle records schema version {} but this tool reads {}":
        "test_rejects_a_pe_oracle_written_against_another_schema, "
        "test_rejects_a_ghidra_oracle_written_against_another_schema",
    "toolchain pin schema version is {} but this tool reads {}":
        "test_rejects_a_pin_written_against_another_schema",
}


class RaiseSiteCensusTests(unittest.TestCase):
    @staticmethod
    def message(node):
        import ast

        if isinstance(node, ast.Constant):
            return node.value
        return "".join(part.value if isinstance(part, ast.Constant) else "{}"
                       for part in node.values)

    def raises(self):
        import ast

        source = (TOOLS_DIR / "capstone_manifest.py").read_text(encoding="utf-8")
        return [node.exc for node in ast.walk(ast.parse(source))
                if isinstance(node, ast.Raise)]

    def test_the_tool_raises_nothing_but_value_errors_and_the_cli_exit(self):
        self.assertEqual(
            sorted({node.func.id for node in self.raises()}),
            ["SystemExit", "ValueError"],
        )

    def test_every_raise_site_in_the_tool_has_a_negative_test(self):
        self.assertEqual(
            sorted(self.message(node.args[0]) for node in self.raises()
                   if node.func.id == "ValueError"),
            sorted(RAISE_SITES),
        )

    def test_every_named_negative_test_exists(self):
        named = {name.strip() for names in RAISE_SITES.values()
                 for name in names.split(",")}
        defined = {name for group in (PinTests, InputRejectionTests)
                   for name in dir(group) if name.startswith("test_rejects_")}
        self.assertEqual(named, defined)


if __name__ == "__main__":
    unittest.main()
