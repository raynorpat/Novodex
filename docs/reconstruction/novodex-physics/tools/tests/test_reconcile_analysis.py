import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import reconcile_analysis  # noqa: E402


IMAGE_BASE = 0x10000000
IMAGE_SHA = "f" * 64
TEXT_RVA = 0x1000
TEXT_SIZE = 0x100
RDATA_RVA = 0x2000
DATA_RVA = 0x3000
RELOC_RVA = 0x4000

DISPATCH_RVA = 0x2020
SOURCE_STRING_RVA = 0x2060
SOURCE_STRING = "\\Epic\\Novodex\\SDKs\\Physics\\src\\Scene.cpp"
SECOND_SOURCE_RVA = 0x2090
SECOND_SOURCE = "\\Epic\\Novodex\\SDKs\\Physics\\src\\Actor.cpp"
THIRD_SOURCE_RVA = 0x20C0
THIRD_SOURCE = "\\Epic\\Novodex\\SDKs\\Physics\\src\\NpActor.cpp"

# The fixture's executable extent. Every byte class the reconciliation has to
# recognise appears once, and the image is laid out the way the real one is:
# the product translation units first, the statically linked runtime above the
# last of them. The call graph is shaped so that each phase rule fires.
#
#   0x1000  A   export NxRayPlaneIntersect (pinned Phase 3) and the entry
#               point. The pin is applied after every layer, so what A calls
#               inherits the phase the layout gave A and not the pinned 3
#   0x1014  B   references Scene.cpp (Phase 7) and installs the dispatch table
#   0x1028  C   an intra-function block of B, reached through the switch table
#   0x1040  D   a call cycle with E. Only E is in B's table, and D sits below
#   0x1050  E   it, so D can be placed only on a pass after E's
#   0x1058  M   called by B (Phase 7) and G (Phase 5), and no span names or
#               brackets it: shared runtime, the only kind there is
#   0x1068  F   references Actor.cpp (Phase 5). An unreferenced run splits it:
#               the block at 0x1078 has no function of its own and must
#               inherit F, not the run
#   0x1080  G   two Phase 5 spans bracket it and two phases call it, from B on
#               7 and F on 5. It is inside a Phase 5 translation unit, so the
#               bracket owns it and the caller spread does not
#   0x1098  H   names no unit of its own either, and Actor.cpp below it and
#               NpActor.cpp above it are both Phase 5, so 5 brackets it
#   0x10b0  I   references NpActor.cpp (Phase 5): the last entry naming a
#               translation unit, and so the floor of the runtime tail. B and H
#               call it from two phases, and its unit outranks both
#   0x10c0  J   _strcspn: a runtime function, by Ghidra Function ID. Its block
#               at 0x10c8 has no Ghidra record of its own, so its proof is
#               carried from the entry rather than restated as evidence
#   0x10d4  K   runtime code above the floor that no Function ID names, which
#               is what the image itself holds 327 of
CHUNKS = ((0x1000, 0x1010), (0x1014, 0x1020), (0x1028, 0x1030), (0x1040, 0x1050),
          (0x1050, 0x1058), (0x1058, 0x1060),
          (0x1068, 0x1070), (0x1078, 0x1080), (0x1080, 0x1090),
          (0x1098, 0x10A8), (0x10B0, 0x10C0), (0x10C0, 0x10C8), (0x10CC, 0x10D4),
          (0x10D4, 0x10DC))
GAPS = (
    # rva, end, proposal, bytes
    (0x1010, 0x1014, "alignment", "cccccccc"),
    (0x1020, 0x1028, "data", "28100010" "2c100010"),
    (0x1030, 0x1040, "code", "8b44240483c001c3" "30100010" "34100010"),
    (0x1060, 0x1068, "data", "0000803f00000000"),
    (0x1070, 0x1078, "code", "8b44240483c001c3"),
    (0x1090, 0x1098, "code", "8d4900" "eb03" "8d4900"),
    (0x10A8, 0x10B0, "code", "5643323058433030"),
    (0x10C8, 0x10CC, "alignment", "cccccccc"),
    (0x10DC, 0x1100, "alignment", "cc" * 36),
)
SWITCH_TABLE = {"jump_rva": "0x00001018", "table_rva": "0x00001020", "entries": 2,
                "targets": ["0x00001028", "0x0000102c"]}
GHIDRA_FUNCTIONS = (
    (0x1000, "NxRayPlaneIntersect", ((0x1000, 0x1010),)),
    (0x1014, "FUN_10001014", ((0x1014, 0x1020), (0x1028, 0x1038))),
    (0x1040, "FUN_10001040", ((0x1040, 0x1050),)),
    (0x1050, "FUN_10001050", ((0x1050, 0x1058),)),
    (0x1058, "FUN_10001058", ((0x1058, 0x1060),)),
    (0x1068, "FUN_10001068", ((0x1068, 0x1070),)),
    (0x1080, "FUN_10001080", ((0x1080, 0x1090),)),
    (0x1098, "FUN_10001098", ((0x1098, 0x10A8),)),
    (0x10B0, "FUN_100010b0", ((0x10B0, 0x10C0),)),
    (0x10C0, "_strcspn", ((0x10C0, 0x10C8),)),
    (0x10D4, "FUN_100010d4", ((0x10D4, 0x10DC),)),
)
# from, to: every call the two oracles agree on.
CALLS = ((0x1009, 0x1040), (0x101C, 0x1058), (0x1044, 0x1050),
         (0x1054, 0x1040), (0x106C, 0x1080), (0x107C, 0x1098),
         (0x1084, 0x1058), (0x109C, 0x10B0))


def hexa(value):
    return f"0x{value:08x}"


def pointer(rva, section, target_rva, target_section, relocated=True):
    return {"rva": rva, "rva_hex": hexa(rva), "section": section,
            "value": IMAGE_BASE + target_rva, "value_hex": hexa(IMAGE_BASE + target_rva),
            "target_rva": target_rva, "target_rva_hex": hexa(target_rva),
            "target_section": target_section, "relocated": relocated}


def reference(from_rva, to_rva, kind):
    return {"from_rva": hexa(from_rva), "to_rva": hexa(to_rva), "to_space": "ram",
            "to_space_offset": None, "type": kind, "operand": 0,
            "source": "ANALYSIS", "primary": True}


def make_pe():
    """Build the PE oracle the fixture image would produce."""
    return {
        "schema_version": reconcile_analysis.SCHEMA_VERSION,
        "image": {"name": "sample.dll", "size": 0x6000, "sha256": IMAGE_SHA,
                  "image_base": IMAGE_BASE, "image_base_hex": hexa(IMAGE_BASE),
                  "entry_point": TEXT_RVA, "entry_point_hex": hexa(TEXT_RVA),
                  "size_of_image": 0x5000, "section_alignment": 0x1000},
        "data_directories": [
            {"index": 0, "name": "export", "rva": 0x2100, "rva_hex": hexa(0x2100),
             "size": 0x40},
            {"index": 5, "name": "basereloc", "rva": RELOC_RVA,
             "rva_hex": hexa(RELOC_RVA), "size": 0x20},
        ],
        "sections": [
            {"name": ".text", "rva": TEXT_RVA, "rva_hex": hexa(TEXT_RVA),
             "virtual_size": TEXT_SIZE, "raw_offset": 0x400, "raw_size": 0x200,
             "characteristics": 0x60000020, "characteristics_hex": "0x60000020",
             "executable": True, "sha256": IMAGE_SHA},
            {"name": ".rdata", "rva": RDATA_RVA, "rva_hex": hexa(RDATA_RVA),
             "virtual_size": 0x200, "raw_offset": 0x600, "raw_size": 0x200,
             "characteristics": 0x40000040, "characteristics_hex": "0x40000040",
             "executable": False, "sha256": IMAGE_SHA},
            {"name": ".data", "rva": DATA_RVA, "rva_hex": hexa(DATA_RVA),
             "virtual_size": 0x200, "raw_offset": 0x800, "raw_size": 0x100,
             "characteristics": 0xC0000040, "characteristics_hex": "0xc0000040",
             "executable": False, "sha256": IMAGE_SHA},
            {"name": ".reloc", "rva": RELOC_RVA, "rva_hex": hexa(RELOC_RVA),
             "virtual_size": 0x20, "raw_offset": 0xA00, "raw_size": 0x200,
             "characteristics": 0x42000040, "characteristics_hex": "0x42000040",
             "executable": False, "sha256": IMAGE_SHA},
        ],
        "executable_intervals": [
            {"section": ".text", "rva": TEXT_RVA, "rva_hex": hexa(TEXT_RVA),
             "end_rva": TEXT_RVA + TEXT_SIZE, "end_rva_hex": hexa(TEXT_RVA + TEXT_SIZE),
             "size": TEXT_SIZE, "raw_offset": 0x400}],
        "executable_bytes": TEXT_SIZE,
        "exports": {"directory_rva": 0x2100, "directory_rva_hex": hexa(0x2100),
                    "name": "sample.dll", "timestamp": 0, "ordinal_base": 1,
                    "named_count": 1,
                    "entries": [{"ordinal": 1, "name": "NxRayPlaneIntersect",
                                 "rva": TEXT_RVA, "rva_hex": hexa(TEXT_RVA),
                                 "forwarder": None}]},
        "imports": [{"dll": "OTHER.dll", "name": "Beta", "ordinal": None, "hint": 1,
                     "hint_name_rva": 0x2180, "hint_name_rva_hex": hexa(0x2180),
                     "iat_rva": 0x2000, "iat_rva_hex": hexa(0x2000)}],
        "relocations": [{"rva": rva, "rva_hex": hexa(rva), "type": 3,
                         "type_name": "HIGHLOW", "block_rva": TEXT_RVA,
                         "block_rva_hex": hexa(TEXT_RVA)}
                        for rva in (0x1020, 0x1024, 0x1038, 0x103C, 0x2010,
                                    0x2020, 0x2024, 0x2028)],
        "resources": [{"type": 16, "name": 1, "language": 1033, "rva": 0x2200,
                       "rva_hex": hexa(0x2200), "size": 0x40, "codepage": 0}],
        "string_scan": {"min_length": 4, "encodings": ["ascii"], "criterion": "fixture"},
        "strings": [{"rva": 0x2040, "rva_hex": hexa(0x2040), "section": ".rdata",
                     "encoding": "ascii", "length": 5, "size": 6, "value": "hello"}]
                   + [{"rva": rva, "rva_hex": hexa(rva), "section": ".rdata",
                       "encoding": "ascii", "length": len(value),
                       "size": len(value) + 1, "value": value}
                      for rva, value in ((SOURCE_STRING_RVA, SOURCE_STRING),
                                         (SECOND_SOURCE_RVA, SECOND_SOURCE),
                                         (THIRD_SOURCE_RVA, THIRD_SOURCE))],
        "pointers": [
            pointer(0x1020, ".text", 0x1028, ".text"),
            pointer(0x1024, ".text", 0x102C, ".text"),
            pointer(0x1038, ".text", 0x1030, ".text"),
            pointer(0x103C, ".text", 0x1034, ".text"),
            pointer(0x2010, ".rdata", DATA_RVA, ".data"),
            pointer(DISPATCH_RVA, ".rdata", 0x1050, ".text"),
            pointer(DISPATCH_RVA + 4, ".rdata", 0x1080, ".text"),
            pointer(DISPATCH_RVA + 8, ".rdata", 0x10B0, ".text"),
        ],
    }


def make_ghidra():
    """Build the Ghidra oracle the fixture image would produce."""
    functions = []
    for rva, name, body in GHIDRA_FUNCTIONS:
        called = [{"rva": hexa(target), "space": "ram", "space_offset": None}
                  for source, target in CALLS
                  if any(start <= source < end for start, end in body)]
        functions.append({
            "rva": hexa(rva), "name": name, "namespace": "Global",
            "prototype": f"int {name}(void)", "calling_convention": "__cdecl",
            "stack_purge": 0, "thunk": False, "thunk_target": None,
            "body": [{"rva": hexa(start), "end_rva": hexa(end), "size": end - start}
                     for start, end in body],
            "body_bytes": sum(end - start for start, end in body), "called": called,
            "decompiler_status": "ok", "decompiler_c": "", "decompiler_error": None})
    references = [reference(source, target, "UNCONDITIONAL_CALL")
                  for source, target in CALLS]
    references += [
        reference(0x1004, DATA_RVA, "READ"),
        reference(0x1006, 0x40, "READ"),
        reference(0x1018, DISPATCH_RVA, "DATA"),
        reference(0x101A, SOURCE_STRING_RVA, "DATA"),
        reference(0x101E, 0x2040, "DATA"),
        reference(0x1068, SECOND_SOURCE_RVA, "DATA"),
        reference(0x10B4, THIRD_SOURCE_RVA, "DATA"),
    ]
    return {
        "schema_version": reconcile_analysis.SCHEMA_VERSION,
        "ghidra": {"version": "12.1.2", "language_id": "x86:LE:32:default",
                   "compiler_spec_id": "windows", "analysis_options": {},
                   "metadata": {"SectionAlignment": "4096"}},
        "decompiler": {"timeout_seconds": 60, "simplification_style": "decompile",
                       "options": {}},
        "image": {"name": "sample.dll", "image_base": hexa(IMAGE_BASE)},
        "symbol_scope": {"description": "fixture", "symbol_table_total": 4},
        "memory_blocks": [
            {"name": "Headers", "rva": hexa(0), "end_rva": hexa(TEXT_RVA),
             "size": TEXT_RVA, "executable": False, "initialized": True},
            {"name": ".text", "rva": hexa(TEXT_RVA), "end_rva": hexa(TEXT_RVA + 0x1000),
             "size": 0x1000, "executable": True, "initialized": True}],
        "type_coverage": {"header_sha256": IMAGE_SHA, "types_parsed": 1,
                          "applied_exports": 1, "applied_vtables": 0,
                          "typed_vtable_slots": 0, "skipped_vtable_slots": 0,
                          "skipped_slot_reasons": [], "unresolved": [],
                          "exports_undeclared_in_headers": [],
                          "non_export_entry_points": ["entry"], "excluded_types": []},
        "functions": functions,
        "instruction_ranges": [
            {"rva": hexa(start), "end_rva": hexa(end), "size": end - start,
             "block": ".text"}
            for start, end in ((0x1000, 0x1010), (0x1014, 0x1020), (0x1028, 0x1038),
                               (0x1040, 0x1060), (0x1068, 0x1070), (0x1078, 0x1080),
                               (0x1080, 0x1090),
                               (0x1098, 0x10A8), (0x10B0, 0x10C8),
                               (0x10D4, 0x10DC))],
        "symbols": [],
        "strings": [{"rva": hexa(0x2040), "data_type": "/string", "length": 6,
                     "value": "hello"}]
                   + [{"rva": hexa(rva), "data_type": "/string",
                       "length": len(value) + 1, "value": value}
                      for rva, value in ((SOURCE_STRING_RVA, SOURCE_STRING),
                                         (SECOND_SOURCE_RVA, SECOND_SOURCE),
                                         (THIRD_SOURCE_RVA, THIRD_SOURCE))],
        "references": references,
        "vtables": [],
        "rtti": [],
        "data": [
            {"rva": hexa(0x40), "data_type": "/dword", "length": 8, "label": None},
            {"rva": hexa(0x1060), "data_type": "/float", "length": 4, "label": None},
            {"rva": hexa(0x1064), "data_type": "/float", "length": 4, "label": None},
            {"rva": hexa(0x2040), "data_type": "/string", "length": 6,
             "label": "s_hello"},
        ],
        "counts": {"functions": len(functions), "instruction_ranges": 10, "symbols": 0,
                   "strings": 4, "references": len(references), "vtables": 0,
                   "rtti": 0, "data": 4, "external_symbols": 1,
                   "non_image_references": 0},
        "coverage": {"executable_bytes": 0x1000, "function_body_bytes": 0x94,
                     "instruction_bytes": 0x94, "decompiled_functions": len(functions),
                     "failed_functions": 0},
    }


def instruction(rva, mnemonic="mov", flow="sequential", target=None, size=4):
    return {"rva": hexa(rva), "size": size, "bytes": "90" * size,
            "mnemonic": mnemonic, "operands": "", "flow": flow, "indirect": False,
            "target_rva": hexa(target) if target is not None else None,
            "stack_delta": 0}


def make_capstone():
    """Build the Capstone oracle the fixture image would produce."""
    gaps = [{"rva": hexa(rva), "end_rva": hexa(end), "size": end - rva,
             "bytes": raw, "proposal": proposal, "swept_instructions": 2,
             "undecodable_offsets": []}
            for rva, end, proposal, raw in GAPS]
    chunks = [{"rva": hexa(rva), "end_rva": hexa(end), "size": end - rva,
               "instructions": 2, "terminator": "return"} for rva, end in CHUNKS]
    instructions = [instruction(rva) for rva, _ in CHUNKS]
    instructions += [instruction(source, "call", "call", target)
                     for source, target in CALLS]
    instructions += [instruction(0x1018, "jmp", "jump", size=7),
                     instruction(0x102C, "ret", "return", size=1)]
    entries = [{"rva": hexa(rva), "sources": ["ghidra_function", "direct_call"],
                "kind": "function"} for rva, _, _ in GHIDRA_FUNCTIONS]
    entries.append({"rva": hexa(0x1028), "sources": ["switch_target"],
                    "kind": "block"})
    entries.append({"rva": hexa(0x1078), "sources": ["direct_branch"],
                    "kind": "block"})
    entries.append({"rva": hexa(0x10CC), "sources": ["direct_branch"],
                    "kind": "block"})
    return {
        "schema_version": reconcile_analysis.SCHEMA_VERSION,
        "capstone": {"version": "5.0.6", "arch": "x86", "mode": "32",
                     "syntax": "intel", "detail": True},
        "image": {"name": "sample.dll", "size": 0x6000, "sha256": IMAGE_SHA,
                  "image_base": hexa(IMAGE_BASE), "entry_point": hexa(TEXT_RVA)},
        "method": {},
        "executable_intervals": [{"section": ".text", "rva": hexa(TEXT_RVA),
                                  "end_rva": hexa(TEXT_RVA + TEXT_SIZE),
                                  "size": TEXT_SIZE}],
        "seed_sources": {},
        "discovery": {"rounds": [], "iterations": [], "rechecked_seeds": 0},
        "entries": sorted(entries, key=lambda entry: entry["rva"]),
        "seeds_outside_executable": [],
        "relocation_uses": [
            {"rva": hexa(0x101B), "target_rva": hexa(0x1020), "use": "displacement"},
            {"rva": hexa(0x1020), "target_rva": hexa(0x1028), "use": "unresolved"},
            {"rva": hexa(0x1024), "target_rva": hexa(0x102C), "use": "unresolved"},
            {"rva": hexa(0x1038), "target_rva": hexa(0x1030), "use": "unresolved"},
            {"rva": hexa(0x103C), "target_rva": hexa(0x1034), "use": "unresolved"},
        ],
        "chunks": chunks,
        "instructions": sorted(instructions, key=lambda record: record["rva"]),
        "undecodable": [],
        "switch_tables": [SWITCH_TABLE],
        "import_thunks": [],
        "tail_calls": [],
        "overlaps": [],
        "coverage_gaps": gaps,
        "counts": {"entries": len(entries), "chunks": len(chunks),
                   "instructions": len(instructions), "descent_undecodable": 0,
                   "sweep_undecodable": 0, "switch_tables": 1, "import_thunks": 0,
                   "tail_calls": 0, "unresolved_indirect_jumps": 0,
                   "coverage_gaps": len(gaps), "overlaps": 0,
                   "seeds_outside_executable": 0, "relocation_uses": 5,
                   "chunks_without_entry": 0},
        "coverage": {"executable_bytes": TEXT_SIZE, "instruction_bytes": 156,
                     "undecodable_bytes": 0, "unclassified_bytes": 100,
                     "proposed_alignment_bytes": 44, "proposed_code_bytes": 40,
                     "proposed_data_bytes": 16, "sweep_undecodable_bytes": 0},
    }


def make_inventory():
    """The committed scaffold the reconciliation reads its pins out of."""
    return {
        "schema_version": reconcile_analysis.SCHEMA_VERSION,
        "pins": {
            "oracle": {"path": "Binaries/sample.dll", "sha256": IMAGE_SHA,
                       "size": 0x6000, "named_exports": 1},
            "link_oracle": {"path": "lib/sample.lib", "sha256": IMAGE_SHA},
            "public_headers": {"root": "include/", "sdk_version": "1.0",
                               "physics_manifest": "h.json",
                               "foundation_manifest": "f.json"},
            "analysis_toolchain": "analysis_toolchain.json",
            "labels": "labels.json",
        },
        "sections": [], "functions": [], "data_objects": [], "exports": [],
        "imports": [],
        "coverage": {"census": {"status": "pending",
                                "code_report": "oracle/coverage.json",
                                "data_report": "oracle/data-coverage.json"},
                     "executable_bytes": 0, "explained_executable_bytes": 0,
                     "unexplained_executable_bytes": 0,
                     "unresolved_executable_targets": 0,
                     "referenced_data_bytes": 0,
                     "unexplained_referenced_data_bytes": 0, "overlaps": 0,
                     "duplicate_ownership": 0},
        "phases": [],
        "gates": {"toolchain_pinned": "pending", "public_headers_pinned": "pending",
                  "pe_manifest": "pending", "ghidra_semantics": "pending",
                  "capstone_corpus": "pending", "phase_1_oracle_census": "pending"},
    }


def reconcile():
    return reconcile_analysis.reconcile(
        pe=make_pe(), ghidra=make_ghidra(), capstone=make_capstone(),
        inventory=make_inventory())


class ReconciliationTests(unittest.TestCase):
    """The fixture reconciles, and its census closes."""

    @classmethod
    def setUpClass(cls):
        cls.result = reconcile()
        cls.rows = {row["rva"]: row for row in cls.result["inventory"]["functions"]}

    def test_every_executable_byte_has_exactly_one_owner(self):
        coverage = self.result["inventory"]["coverage"]
        self.assertEqual(coverage["executable_bytes"], TEXT_SIZE)
        self.assertEqual(coverage["unexplained_executable_bytes"], 0)
        self.assertEqual(coverage["explained_executable_bytes"], TEXT_SIZE)
        self.assertEqual(coverage["duplicate_ownership"], 0)
        self.assertEqual(coverage["overlaps"], 0)

    def test_the_census_passes_and_the_gate_closes(self):
        inventory = self.result["inventory"]
        self.assertEqual(inventory["coverage"]["census"]["status"], "pass")
        self.assertEqual(inventory["gates"]["phase_1_oracle_census"], "pass")

    def test_stable_ids_are_assigned_from_sorted_entry_rvas(self):
        for table, prefix in (("functions", "phys_fn_"),
                              ("data_objects", "phys_data_")):
            rows = self.result["inventory"][table]
            self.assertEqual(
                [row["id"] for row in rows],
                [f"{prefix}{index:06d}" for index in range(1, len(rows) + 1)])
            self.assertEqual([int(row["rva"], 16) for row in rows],
                             sorted(int(row["rva"], 16) for row in rows))

    def test_a_walked_switch_table_is_data_owned_by_the_jumping_function(self):
        tables = [row for row in self.result["inventory"]["data_objects"]
                  if row["type"] == "switch_table"]
        self.assertEqual([(row["rva"], row["size"]) for row in tables],
                         [("0x00001020", 8)])
        owner = {row["id"]: row for row in self.result["inventory"]["functions"]}
        self.assertEqual(owner[tables[0]["owner"]]["rva"], "0x00001014")

    def test_bytes_only_ghidra_decoded_are_owned_with_both_oracle_references(self):
        self.assertIn("0x00001030", self.rows)
        self.assertEqual(self.rows["0x00001030"]["size"], 8)
        self.assertIn("functions/1", self.rows["0x00001030"]["ghidra_ref"])
        self.assertIn("coverage_gaps/", self.rows["0x00001030"]["capstone_ref"])
        self.assertEqual(
            self.result["coverage"]["layers"]["ghidra_only_instruction_bytes"], 8)

    def test_a_run_of_unresolved_slots_is_a_derived_switch_table(self):
        tables = [row for row in self.result["inventory"]["data_objects"]
                  if row["type"] == "derived_switch_table"]
        self.assertEqual([(row["rva"], row["size"]) for row in tables],
                         [("0x00001038", 8)])

    def test_a_lone_unresolved_slot_is_not_claimed_as_a_table(self):
        capstone = make_capstone()
        capstone["relocation_uses"] = [
            use for use in capstone["relocation_uses"]
            if use["rva"] != hexa(0x103C)]
        result = reconcile_analysis.reconcile(
            pe=make_pe(), ghidra=make_ghidra(), capstone=capstone,
            inventory=make_inventory())
        self.assertEqual([row for row in result["inventory"]["data_objects"]
                          if row["type"] == "derived_switch_table"], [])
        self.assertEqual(
            result["coverage"]["target_discovery"][
                "relocation_sites_too_lone_to_be_a_table"],
            ["0x00001038"])

    def test_a_decoded_block_after_an_unreferenced_run_inherits_the_code_owner(self):
        # 0x1078 is a decoded chunk with no function of its own, right after the
        # unreferenced run at 0x1070. It must inherit F, not the run, or it would
        # carry the run proof that nothing in any oracle reaches these bytes.
        row = self.rows["0x00001078"]
        self.assertEqual(row["notes"], "continuation of the entry at 0x00001068")
        self.assertEqual(row["kind"], "code")
        self.assertIsNone(row["static_proof"])
        self.assertEqual(row["phase"], self.rows["0x00001068"]["phase"])

    def test_a_data_object_below_the_executable_extent_sorts_first(self):
        rows = self.result["inventory"]["data_objects"]
        self.assertEqual((rows[0]["rva"], rows[0]["id"]),
                         ("0x00000040", "phys_data_000001"))
        # The .text rows are built first, so only a global sort puts them after it.
        self.assertGreater(
            min(int(row["rva"], 16) for row in rows if row["section"] == ".text"),
            int(rows[0]["rva"], 16))

    def test_unresolved_executable_targets_is_the_measured_count(self):
        coverage = self.result["inventory"]["coverage"]
        self.assertEqual(
            coverage["unresolved_executable_targets"],
            self.result["coverage"]["target_discovery"]["unresolved_targets"])

    def test_padding_run_alignment_is_published_for_the_reader(self):
        coverage = self.result["coverage"]
        # Three of the four end on a 4-byte boundary; the fourth is the run the
        # alignment jump skips, which ends mid-word at 0x1093 by construction.
        self.assertEqual(coverage["padding_runs"], 5)
        self.assertEqual(coverage["padding_runs_ending_on_an_alignment_boundary"], 4)

    def test_a_memory_block_reference_names_the_block_that_holds_the_row(self):
        blocks = make_ghidra()["memory_blocks"]
        for row in self.result["inventory"]["functions"]:
            if "#/memory_blocks/" not in row["ghidra_ref"]:
                continue
            block = blocks[int(row["ghidra_ref"].rsplit("/", 1)[1])]
            start = int(row["rva"], 16)
            self.assertLessEqual(int(block["rva"], 16), start, row["id"])
            self.assertLessEqual(start + row["size"], int(block["end_rva"], 16), row["id"])

    def test_no_proof_asserts_evidence_the_rows_own_reference_denies(self):
        for row in self.result["inventory"]["functions"]:
            if (row["static_proof"] or "").startswith("Ghidra Function ID"):
                self.assertNotIn("memory_blocks", row["ghidra_ref"], row["id"])

    def test_a_proof_that_cannot_hold_at_these_bytes_is_carried_not_restated(self):
        # 0x10c8 continues _strcspn, but Ghidra records nothing there, so the
        # row may not restate the Function ID name as evidence about itself.
        row = self.rows["0x000010cc"]
        # The block named must be the one that contains the row, not block 0.
        self.assertTrue(row["ghidra_ref"].endswith("#/memory_blocks/1"), row["ghidra_ref"])
        self.assertTrue(
            row["static_proof"].startswith(
                "carried forward from the entry at 0x000010c0"),
            row["static_proof"])
        self.assertIn("Ghidra Function ID names it _strcspn", row["static_proof"])

    def test_padding_is_a_compiler_artifact_that_claims_no_product_source(self):
        self.assertEqual(self.rows["0x00001010"]["kind"], "compiler_artifact")
        self.assertIsNone(self.rows["0x00001010"]["source"])
        self.assertTrue(self.rows["0x00001010"]["static_proof"])

    def test_a_jump_over_alignment_is_padding_and_says_so(self):
        row = self.rows["0x00001093"]
        self.assertEqual((row["size"], row["kind"]), (2, "compiler_artifact"))
        self.assertIn("first byte after the alignment run", row["static_proof"])

    def test_no_rtti_is_invented_for_a_binary_that_has_none(self):
        report = self.result["data_coverage"]
        self.assertEqual(report["vtables"], [])
        self.assertEqual(report["rtti"], [])
        self.assertEqual(report["dispatch_structures"]["rtti_records_recovered"], 0)
        self.assertEqual(report["dispatch_structures"]["vtables_recovered"], 0)

    def test_every_export_is_owned_by_a_function_row(self):
        inventory = self.result["inventory"]
        ids = {row["id"] for row in inventory["functions"]}
        self.assertEqual(len(inventory["exports"]), 1)
        for row in inventory["exports"]:
            self.assertIn(row["function_id"], ids)

    def test_every_row_names_exactly_one_declared_phase(self):
        inventory = self.result["inventory"]
        declared = {row["phase"] for row in inventory["phases"]}
        for row in inventory["functions"] + inventory["data_objects"]:
            self.assertIn(row["phase"], declared)

    def test_the_catch_all_data_class_is_counted_apart_from_the_proven_ones(self):
        report = self.result["data_coverage"]
        self.assertEqual(report["bounded_referenced_bytes"],
                         report["bytes_by_type"].get("code_addressed_global", 0))
        self.assertNotIn("code_addressed_global", str(report["explained_referenced_bytes"]))
        self.assertEqual(report["unexplained_referenced_addresses"],
                         report["unexplained_referenced_bytes"])


class PhaseAssignmentTests(unittest.TestCase):
    """Each rule that decides the phase column, driven end to end."""

    @classmethod
    def setUpClass(cls):
        cls.result = reconcile()
        cls.rows = {row["rva"]: row for row in cls.result["inventory"]["functions"]}

    def phase(self, rva):
        return self.rows[hexa(rva)]["phase"]

    def test_a_pinned_export_lands_on_the_phase_the_plan_locks(self):
        self.assertEqual(reconcile_analysis.EXPORT_PHASES["NxRayPlaneIntersect"], 3)
        self.assertEqual(self.phase(0x1000), 3)
        self.assertEqual(self.rows["0x00001000"]["label"], "NxRayPlaneIntersect")

    def test_a_source_file_reference_seeds_the_component_it_names(self):
        self.assertEqual(self.phase(0x1014), 7)
        self.assertEqual(self.rows["0x00001014"]["source"], "Physics/src/Scene.cpp")
        seeds = self.result["coverage"]["phase_seeds"]
        self.assertEqual(seeds["source_references"], 3)
        self.assertEqual(seeds["source_references_with_no_owner"], 0)

    def test_a_source_reference_outside_every_ghidra_body_still_seeds(self):
        # The reference moves into the run only Ghidra decoded, which no Ghidra
        # function body covers. Resolving through the census's owner map keeps it.
        ghidra = make_ghidra()
        for entry in ghidra["references"]:
            if entry["to_rva"] == hexa(SOURCE_STRING_RVA):
                entry["from_rva"] = hexa(0x1034)
        result = reconcile_analysis.reconcile(
            pe=make_pe(), ghidra=ghidra, capstone=make_capstone(),
            inventory=make_inventory())
        seeds = result["coverage"]["phase_seeds"]
        self.assertEqual(seeds["source_references_with_no_owner"], 0)
        rows = {row["rva"]: row for row in result["inventory"]["functions"]}
        self.assertEqual(rows["0x00001030"]["source"], "Physics/src/Scene.cpp")

    def test_a_dispatch_table_is_recovered_from_the_pointer_table(self):
        structures = self.result["data_coverage"]["dispatch_structures"]
        self.assertEqual(structures["pointer_tables_recovered"], 1)
        self.assertEqual(structures["pointer_table_slots"], 3)
        table = next(row for row in self.result["inventory"]["data_objects"]
                     if row["type"] == "dispatch_table")
        self.assertEqual((table["rva"], table["size"]), (hexa(DISPATCH_RVA), 12))

    def test_an_entry_inside_a_translation_units_span_is_owned_by_it(self):
        # Three entries name a file, and each one bounds the span its unit
        # occupies. B is Scene.cpp on Phase 7; F and I are Phase 5 files.
        self.assertEqual(
            [self.phase(entry) for entry in (0x1014, 0x1068, 0x10B0)], [7, 5, 5])
        self.assertEqual(self.rows["0x00001068"]["source"], "Physics/src/Actor.cpp")
        self.assertEqual(self.rows["0x000010b0"]["source"], "Physics/src/NpActor.cpp")

    def test_an_entry_between_two_spans_of_one_phase_is_bracketed_by_it(self):
        # H names no unit of its own, but Actor.cpp below it and NpActor.cpp
        # above it are both Phase 5, so the unit it belongs to is bracketed by
        # 5. The only call that reaches it comes from B on Phase 7: the layout
        # evidence outranks the call, which is the whole point of the layer.
        self.assertEqual(self.phase(0x1098), 5)
        self.assertEqual(self.phase(0x1014), 7)

    def test_dispatch_edges_carry_the_installers_phase_to_its_methods(self):
        # E is reached only through the table B installs, and no span names or
        # brackets it. Without that edge nothing reaches it and it falls to the
        # nearest span instead, which is Actor.cpp on Phase 5.
        self.assertEqual(self.phase(0x1050), self.phase(0x1014))
        self.assertEqual(self.phase(0x1050), 7)

    def test_a_call_cycle_is_phased_from_the_one_caller_that_reaches_it(self):
        # D and E call each other and only E is in B's table. D sits below E,
        # so a pass that visits the entries in address order cannot place it
        # from E on that same pass: reaching the answer takes a second one. One
        # pass would leave D to the weakest layout rule and on Phase 5.
        self.assertEqual(self.phase(0x1040), self.phase(0x1050))
        self.assertEqual(self.phase(0x1040), 7)

    def test_a_callee_two_phases_call_falls_to_shared_runtime(self):
        # M is called from B (Scene.cpp, Phase 7) and from G (Phase 5), and it
        # sits in the gap between a Phase 7 span and a Phase 5 one, so no unit
        # names it and none brackets it either. Code with no translation-unit
        # home that several phases call is what shared runtime means.
        self.assertEqual(self.phase(0x1058), reconcile_analysis.SHARED_PHASE)

    def test_a_named_translation_unit_outranks_the_shared_override(self):
        # B (Phase 7) and H (Phase 5) both call I, which is the two-phase shape
        # the shared override fires on - but NpActor.cpp names I, and a unit
        # that names an entry outranks the callers that reach it.
        self.assertEqual(self.phase(0x10B0), 5)
        self.assertEqual({self.phase(0x1014), self.phase(0x1098)}, {7, 5})

    def test_a_bracketing_pair_of_spans_outranks_the_shared_override(self):
        # G names no unit of its own, but Actor.cpp below it and NpActor.cpp
        # above it are both Phase 5, so it sits inside a Phase 5 translation
        # unit and Phase 5 reconstructs it with the rest of that unit. B on
        # Phase 7 and F on Phase 5 both call it, which is the two-phase shape
        # the override fires on: being called from several subsystems is
        # ordinary C++, not evidence of a separate owner, so the bracket wins.
        self.assertEqual(self.phase(0x1080), 5)
        self.assertEqual({self.phase(0x1014), self.phase(0x1068)}, {7, 5})

    def test_the_shared_override_reads_the_phases_it_was_handed(self):
        # 0x300 is called by 0x100 and by 0x200, which the layers left on one
        # phase apiece, so one phase owns it. The same pass moves 0x200 to
        # shared runtime: reading the phases as the pass rewrites them, rather
        # than as it found them, would carry 0x300 out with it.
        phases, provenance = reconcile_analysis.assign_phases(
            {0x100, 0x200, 0x300, 0x400, 0x600},
            {(0x100, 0x200), (0x400, 0x200), (0x200, 0x300), (0x100, 0x300)},
            {0x100: "Scene.cpp", 0x600: "Actor.cpp"}, {}, {}, 0x600)
        self.assertEqual((phases[0x200], provenance[0x200]),
                         (reconcile_analysis.SHARED_PHASE, "shared_by_callers"))
        self.assertEqual((phases[0x300], provenance[0x300]), (7, "callers"))

    def test_an_export_pin_binds_the_export_and_not_what_it_calls(self):
        # A is pinned to Phase 3 and calls D. The pin is a plan decision, not
        # evidence about the code, so it lands on A alone and D keeps the
        # Phase 7 the call graph gave it. Pinning before the layers instead
        # would carry the 3 into D and push E out to shared runtime.
        self.assertEqual(self.phase(0x1000), 3)
        self.assertEqual((self.phase(0x1040), self.phase(0x1050)), (7, 7))

    def test_a_caller_the_override_moves_leaves_its_callees_where_it_put_them(self):
        # The shared override runs after the caller layer, so an entry can hold
        # a phase inherited from a caller the override then moves to Phase 2.
        # The inherited phase is the one the evidence gave and it stands - but
        # `callers` then names a caller that no longer carries it, which is a
        # trap for anyone auditing a row by following its provenance.
        files = {0x100: "Scene.cpp", 0x600: "Actor.cpp"}
        nodes = {0x100, 0x200, 0x300, 0x400, 0x600}
        edges = {(0x100, 0x200), (0x400, 0x200), (0x200, 0x300)}
        phases, provenance = reconcile_analysis.assign_phases(
            nodes, edges, files, {}, {}, max(files))
        self.assertEqual((phases[0x200], provenance[0x200]),
                         (reconcile_analysis.SHARED_PHASE, "shared_by_callers"))
        self.assertEqual((phases[0x300], provenance[0x300]), (7, "callers"))
        # 0x300 has exactly one caller and it is no longer on 0x300's phase.
        self.assertEqual([phases[source] for source, target in edges
                          if target == 0x300], [reconcile_analysis.SHARED_PHASE])

    def test_an_entry_no_rule_reaches_takes_the_nearer_translation_unit(self):
        # Drop the reference that names B as the table's installer and the only
        # thing left reaching E is D, which is itself unplaced. No span names or
        # brackets E either, so it falls to the span nearest it by address:
        # Actor.cpp above, on Phase 5, and not Scene.cpp below, on Phase 7.
        ghidra = make_ghidra()
        ghidra["references"] = [entry for entry in ghidra["references"]
                                if entry["to_rva"] != hexa(DISPATCH_RVA)]
        result = reconcile_analysis.reconcile(
            pe=make_pe(), ghidra=ghidra, capstone=make_capstone(),
            inventory=make_inventory())
        rows = {row["rva"]: row for row in result["inventory"]["functions"]}
        self.assertEqual(rows["0x00001050"]["phase"], 5)
        self.assertEqual(
            result["coverage"]["phase_seeds"]["entries_phased_by"]["layout_adjacency"],
            1)
        # D is left reached by A on 7 and E on 5, so the override claims it.
        self.assertEqual(rows["0x00001040"]["phase"],
                         reconcile_analysis.SHARED_PHASE)

    def test_a_runtime_function_is_a_compiler_artifact_in_the_artifact_phase(self):
        row = self.rows["0x000010c0"]
        self.assertEqual(row["kind"], "compiler_artifact")
        self.assertEqual(row["phase"], reconcile_analysis.ARTIFACT_PHASE)
        self.assertEqual(row["label"], "_strcspn")
        self.assertIsNone(row["source"])

    def test_code_above_the_last_translation_unit_is_runtime_no_one_named(self):
        # K sits above I, the last entry naming a unit, and no Function ID
        # names it. The image holds 327 rows like it, and calling them product
        # code was the defect this layer closes.
        row = self.rows["0x000010d4"]
        self.assertEqual(row["kind"], "compiler_artifact")
        self.assertEqual(row["phase"], reconcile_analysis.ARTIFACT_PHASE)
        self.assertIsNone(row["source"])
        self.assertIn("no product translation unit reaches these bytes",
                      row["static_proof"])
        self.assertIn(hexa(0x10B0), row["static_proof"])

    def test_every_entry_records_how_its_phase_was_decided(self):
        coverage = self.result["coverage"]
        self.assertEqual(coverage["phase_seeds"]["entries_phased_by"],
                         {"callers": 2, "enclosed_by_one_phase": 2, "export_pin": 1,
                          "runtime_artifact": 2, "runtime_tail": 1,
                          "shared_by_callers": 1, "translation_unit": 3})
        self.assertEqual(coverage["entries_per_phase_by_provenance"],
                         {"2": {"shared_by_callers": 1}, "3": {"export_pin": 1},
                          "5": {"enclosed_by_one_phase": 2, "translation_unit": 2},
                          "7": {"callers": 2, "translation_unit": 1},
                          "8": {"runtime_artifact": 2, "runtime_tail": 1}})
        # The two counters above are totals the emitter reports about itself.
        # An emitter that stamped one rule on every row would still publish a
        # legal inventory and still validate, so name the rule each of the
        # eleven code rows carries and let the totals be a cross-check.
        self.assertEqual({row["rva"]: row["phase_provenance"]
                          for row in self.result["inventory"]["functions"]
                          if row["kind"] == "code"},
                         {"0x00001000": "export_pin",
                          "0x00001014": "translation_unit",
                          "0x00001028": "translation_unit",
                          "0x00001030": "translation_unit",
                          "0x00001040": "callers",
                          "0x00001050": "callers",
                          "0x00001058": "shared_by_callers",
                          "0x00001068": "translation_unit",
                          "0x00001078": "translation_unit",
                          "0x00001080": "enclosed_by_one_phase",
                          "0x00001098": "enclosed_by_one_phase",
                          "0x000010b0": "translation_unit"})
        # The runtime rows and the alignment between them are the other half of
        # the column, and they are the half a mutation can hide in: they never
        # appear in `entries_phased_by` at all.
        self.assertEqual({row["rva"]: row["phase_provenance"]
                          for row in self.result["inventory"]["functions"]
                          if row["kind"] == "compiler_artifact"},
                         {"0x00001010": "padding", "0x00001070": "runtime_artifact",
                          "0x00001090": "padding", "0x00001093": "padding",
                          "0x00001095": "padding", "0x000010c0": "runtime_artifact",
                          "0x000010c8": "padding", "0x000010cc": "runtime_artifact",
                          "0x000010d4": "runtime_tail", "0x000010dc": "padding"})

    def test_every_data_object_records_how_its_phase_was_decided(self):
        # Data carries three rules, not one. An object inside `.text` is
        # owned by the function it sits in and inherits that function's
        # provenance; a PE structure is placed by its own layout; everything
        # else is placed by the code that reads it. Reporting one of the three
        # everywhere leaves the phase column intact and the census passing.
        self.assertEqual({row["rva"]: (row["section"], row["phase_provenance"])
                          for row in self.result["inventory"]["data_objects"]},
                         {"0x00000040": ("headers", "pe_structure"),
                          "0x00001020": (".text", "translation_unit"),
                          "0x00001038": (".text", "translation_unit"),
                          "0x00001060": (".text", "shared_by_callers"),
                          "0x00001064": (".text", "shared_by_callers"),
                          "0x000010a8": (".text", "enclosed_by_one_phase"),
                          "0x00002000": (".rdata", "pe_structure"),
                          "0x00002020": (".rdata", "reading_sites"),
                          "0x00002040": (".rdata", "reading_sites"),
                          "0x00002060": (".rdata", "reading_sites"),
                          "0x00002090": (".rdata", "reading_sites"),
                          "0x000020c0": (".rdata", "reading_sites"),
                          "0x00003000": (".data", "reading_sites")})

    def test_a_data_object_takes_the_phase_of_the_code_that_reads_it(self):
        table = next(row for row in self.result["inventory"]["data_objects"]
                     if row["type"] == "dispatch_table")
        self.assertEqual(table["phase"], 7)
        self.assertEqual(self.phase(0x1014), 7)

    def test_the_dependency_graph_records_the_calls_the_oracles_agree_on(self):
        # Eight calls both oracles record, plus the three dispatch-table edges.
        self.assertEqual(self.result["dependencies"].count(" -> "), 11)


def fixture_ruling(slots=("object_model", "collision", "collision")):
    """A shape ruling over the fixture's one dispatch table, B's, holding E, G and I."""
    return {"phase_of": {"collision": 3, "object_model": 5},
            "tables": [{"class": "FIXTURE", "rva": hexa(DISPATCH_RVA), "slots": list(slots)}],
            "members": [], "exceptions": []}


class SlotRulingPhaseTests(unittest.TestCase):
    """The shape ruling is a phase layer, read through the validator's own resolver."""

    @classmethod
    def setUpClass(cls):
        cls.result = reconcile_analysis.reconcile(
            pe=make_pe(), ghidra=make_ghidra(), capstone=make_capstone(),
            inventory=make_inventory(), ruling=fixture_ruling())
        cls.rows = {row["rva"]: row for row in cls.result["inventory"]["functions"]}

    def placed(self, rva):
        row = self.rows[hexa(rva)]
        return row["phase"], row["phase_provenance"]

    def test_a_ruled_slot_outranks_the_rule_that_placed_its_target(self):
        # E was placed by its installer's phase, G by a Phase 5 bracket and I by
        # its own translation unit; each slot's ruling replaces all three.
        self.assertEqual(self.placed(0x1050), (5, "slot_ruling"))
        self.assertEqual(self.placed(0x1080), (3, "slot_ruling"))
        self.assertEqual(self.placed(0x10B0), (3, "slot_ruling"))

    def test_a_ruled_row_seeds_the_caller_layer(self):
        # D is called from A, on Phase 7 before its pin, and from E. Without the
        # ruling both are 7 and D follows them; with E ruled to 5 its callers
        # disagree and D falls to shared runtime, which only happens if the caller
        # layer read E's ruled phase.
        self.assertEqual(self.placed(0x1040), (2, "shared_by_callers"))
        rows = {row["rva"]: row for row in reconcile()["inventory"]["functions"]}
        self.assertEqual((rows[hexa(0x1040)]["phase"], rows[hexa(0x1040)]["phase_provenance"]),
                         (7, "callers"))

    def test_the_shared_override_leaves_a_ruled_row_where_the_ruling_put_it(self):
        # G is called from B on 7 and F on 5, the shape the override fires on.
        self.assertEqual(self.placed(0x1080)[1], "slot_ruling")

    def test_without_a_ruling_the_layers_are_unchanged(self):
        rows = {row["rva"]: row for row in reconcile()["inventory"]["functions"]}
        self.assertEqual([rows[hexa(rva)]["phase_provenance"] for rva in (0x1050, 0x1080, 0x10B0)],
                         ["callers", "enclosed_by_one_phase", "translation_unit"])


class InputRejectionTests(unittest.TestCase):
    """One field moves on a fixture the tests above prove valid."""

    def rejects(self, mutate, message):
        oracles = {"pe": make_pe(), "ghidra": make_ghidra(),
                   "capstone": make_capstone(), "inventory": make_inventory()}
        mutate(oracles)
        with self.assertRaisesRegex(ValueError, message):
            reconcile_analysis.reconcile(**oracles)

    def test_rejects_a_shape_ruling_that_does_not_resolve(self):
        # A fourth slot past the end of B's table names no relocated pointer.
        self.rejects(lambda o: o.__setitem__("ruling", fixture_ruling(
            ("object_model", "collision", "collision", "collision"))),
            "the shape ruling does not resolve against the PE oracle: shape ruling "
            "table FIXTURE slot 3 at 0x0000202c is not a relocated pointer")

    def test_rejects_a_pe_oracle_written_against_another_schema(self):
        self.rejects(lambda o: o["pe"].__setitem__("schema_version", 2),
                     "the pe oracle records schema version 2 but this tool reads 1")

    def test_rejects_a_ghidra_oracle_written_against_another_schema(self):
        self.rejects(lambda o: o["ghidra"].__setitem__("schema_version", 2),
                     "the ghidra oracle records schema version 2 but this tool reads 1")

    def test_rejects_a_capstone_oracle_written_against_another_schema(self):
        self.rejects(lambda o: o["capstone"].__setitem__("schema_version", 2),
                     "the capstone oracle records schema version 2 but this tool "
                     "reads 1")

    def test_rejects_a_capstone_corpus_built_from_another_image(self):
        self.rejects(lambda o: o["capstone"]["image"].__setitem__("sha256", "0" * 64),
                     "the capstone oracle describes image 0{64} but the PE oracle "
                     "describes f{64}")

    def test_rejects_an_inventory_pinning_another_image(self):
        self.rejects(lambda o: o["inventory"]["pins"]["oracle"].__setitem__(
            "sha256", "1" * 64),
            "the inventory pins image 1{64} but the PE oracle describes f{64}")

    def test_rejects_an_executable_extent_the_corpus_disagrees_with(self):
        self.rejects(lambda o: o["capstone"]["executable_intervals"][0].__setitem__(
            "size", 240),
            "the capstone oracle covers 240 executable bytes but the PE oracle "
            "records 256")

    def test_rejects_a_corpus_whose_chunks_and_gaps_do_not_tile_the_extent(self):
        self.rejects(lambda o: o["capstone"]["coverage_gaps"].pop(),
                     "the capstone corpus accounts for 220 of 256 executable bytes")

    def test_rejects_translation_unit_spans_that_interleave(self):
        # I moves from NpActor.cpp onto Scene.cpp, so Scene.cpp's span reaches
        # from B to I and swallows Actor.cpp. Every layout rule reads the spans
        # as disjoint, so this has to stop the run rather than be assigned from.
        def mutate(oracles):
            for entry in oracles["ghidra"]["references"]:
                if entry["to_rva"] == hexa(THIRD_SOURCE_RVA):
                    entry["to_rva"] = hexa(SOURCE_STRING_RVA)

        self.rejects(mutate,
                     "translation unit Scene.cpp occupies 0x00001014 to "
                     "0x000010b0, which Actor.cpp interleaves at 0x00001068")

    def test_rejects_overlapping_function_ownership(self):
        self.rejects(lambda o: o["ghidra"]["functions"][0]["body"].append(
            {"rva": hexa(0x1014), "end_rva": hexa(0x1020), "size": 0xC}),
            "Ghidra function 0x00001014 body overlaps 0x00001000")

    def test_rejects_an_executable_target_no_owned_entry_explains(self):
        self.rejects(lambda o: o["capstone"]["instructions"][1].__setitem__(
            "target_rva", hexa(0x1012)),
            "1 recovered executable targets are unresolved, the first being "
            "0x00001012")

    def test_rejects_a_gap_byte_string_that_does_not_match_its_extent(self):
        self.rejects(lambda o: o["capstone"]["coverage_gaps"][0].__setitem__(
            "bytes", "cccc"),
            "coverage gap 0x00001010 publishes 2 bytes for an extent of 4")

    def test_rejects_a_census_that_leaves_an_executable_byte_unowned(self):
        self.rejects(lambda o: o["capstone"]["coverage_gaps"][4].__setitem__(
            "undecodable_offsets", [7]),
            "the census leaves 8 executable bytes unexplained, the first run at "
            "0x00001070")


class SelfAuditTests(unittest.TestCase):
    """Checks the reconciliation runs against its own output."""

    def test_rejects_a_semantic_label_without_evidence(self):
        with self.assertRaisesRegex(ValueError,
                                    "label 'phys_fn_000001' records no evidence"):
            reconcile_analysis.check_label_ledger(
                [{"id": "phys_fn_000001", "label": "Alpha",
                  "label_confidence": "semantic"}],
                [{"id": "phys_fn_000001", "label": "Alpha", "confidence": "semantic",
                  "evidence": "", "reason": "an export names it"}])

    def test_rejects_a_semantic_label_the_ledger_does_not_carry(self):
        with self.assertRaisesRegex(ValueError,
                                    "'phys_fn_000001' carries a semantic label "
                                    "without a ledger entry"):
            reconcile_analysis.check_label_ledger(
                [{"id": "phys_fn_000001", "label": "Alpha",
                  "label_confidence": "semantic"}], [])

    def test_rejects_duplicate_stable_ids(self):
        with self.assertRaisesRegex(ValueError,
                                    "stable ID 'phys_fn_000001' is assigned more "
                                    "than once"):
            reconcile_analysis.check_stable_ids(
                [{"id": "phys_fn_000001"}, {"id": "phys_fn_000001"}])

    def test_rejects_a_row_missing_an_oracle_reference(self):
        with self.assertRaisesRegex(
                ValueError, "function 'phys_fn_000001' records no capstone_ref"):
            reconcile_analysis.check_row_references(
                [{"id": "phys_fn_000001", "ghidra_ref": "g", "capstone_ref": ""}])

    def test_rejects_a_passing_census_with_unresolved_targets(self):
        with self.assertRaisesRegex(
                ValueError, "census passes but unresolved_executable_targets is 2"):
            reconcile_analysis.check_census({
                "unexplained_executable_bytes": 0, "unresolved_executable_targets": 2,
                "unexplained_referenced_data_bytes": 0, "overlaps": 0,
                "duplicate_ownership": 0})

    def test_rejects_a_passing_census_with_unexplained_bytes(self):
        with self.assertRaisesRegex(
                ValueError, "census passes but unexplained_executable_bytes is 4"):
            reconcile_analysis.check_census({
                "unexplained_executable_bytes": 4, "unresolved_executable_targets": 0,
                "unexplained_referenced_data_bytes": 0, "overlaps": 0,
                "duplicate_ownership": 0})

    def test_rejects_a_function_entry_no_row_owns(self):
        with self.assertRaisesRegex(
                ValueError,
                "2 function entries the evidence names are not owned rows, the "
                "first being 0x00001040"):
            reconcile_analysis.check_entries_are_owned([0x1040, 0x1080])


class ReconcileAnalysisCliTests(unittest.TestCase):
    def run_cli(self, directory, **overrides):
        root = Path(directory)
        (root / "oracle" / "ghidra").mkdir(parents=True, exist_ok=True)
        (root / "oracle" / "capstone").mkdir(parents=True, exist_ok=True)
        for name, payload in (("oracle/pe.json", make_pe()),
                              ("oracle/ghidra/manifest.json", make_ghidra()),
                              ("oracle/capstone/manifest.json", make_capstone()),
                              ("inventory.json", make_inventory()),
                              ("labels.json", {"schema_version": 1, "labels": []})):
            (root / name).write_text(json.dumps(payload), encoding="utf-8")
        arguments = {"--pe": str(root / "oracle/pe.json"),
                     "--ghidra": str(root / "oracle/ghidra/manifest.json"),
                     "--capstone": str(root / "oracle/capstone/manifest.json"),
                     "--inventory": str(root / "inventory.json"),
                     "--labels": str(root / "labels.json")}
        arguments.update(overrides)
        command = [sys.executable, str(TOOLS_DIR / "reconcile_analysis.py")]
        for flag, value in arguments.items():
            command += [flag, value]
        return subprocess.run(command, capture_output=True, text=True, check=False), root

    def test_writes_every_artifact_and_reports_its_measurements(self):
        with tempfile.TemporaryDirectory() as directory:
            result, root = self.run_cli(directory)
            self.assertEqual(result.returncode, 0, result.stderr)
            for name in ("oracle/coverage.json", "oracle/data-coverage.json",
                         "oracle/dependencies.dot", "inventory.json", "labels.json"):
                self.assertTrue((root / name).exists(), name)
            inventory = json.loads((root / "inventory.json").read_text(encoding="utf-8"))
        self.assertIn("census=pass", result.stdout)
        self.assertIn("unexplained_executable_bytes=0", result.stdout)
        self.assertEqual(inventory["coverage"]["executable_bytes"], TEXT_SIZE)

    def test_the_written_inventory_satisfies_the_committed_validator(self):
        import validate_inventory

        with tempfile.TemporaryDirectory() as directory:
            result, root = self.run_cli(directory)
            self.assertEqual(result.returncode, 0, result.stderr)
            inventory = json.loads((root / "inventory.json").read_text(encoding="utf-8"))
            labels = json.loads((root / "labels.json").read_text(encoding="utf-8"))
        self.assertEqual(validate_inventory.validate_inventory(inventory), [])
        self.assertEqual(validate_inventory.validate_labels(inventory, labels), [])

    def test_the_run_is_idempotent(self):
        names = ("inventory.json", "labels.json", "oracle/coverage.json",
                 "oracle/data-coverage.json", "oracle/dependencies.dot")
        with tempfile.TemporaryDirectory() as directory:
            result, root = self.run_cli(directory)
            self.assertEqual(result.returncode, 0, result.stderr)
            written = {name: (root / name).read_bytes() for name in names}
            again, _ = self.run_cli(directory)
            self.assertEqual(again.returncode, 0, again.stderr)
            for name in names:
                self.assertEqual((root / name).read_bytes(), written[name], name)

    def test_unreadable_input_reports_cli_error_without_traceback(self):
        with tempfile.TemporaryDirectory() as directory:
            result, _ = self.run_cli(
                directory, **{"--pe": str(Path(directory) / "missing.json")})
        self.assertEqual(result.returncode, 2)
        self.assertIn("error:", result.stderr)
        self.assertNotIn("Traceback", result.stderr)


# Every message the tool can raise, against the test that drives it. A new raise
# site fails this census until it is added here with a test to match.
RAISE_SITES = {
    "the {} oracle records schema version {} but this tool reads {}":
        "test_rejects_a_pe_oracle_written_against_another_schema, "
        "test_rejects_a_ghidra_oracle_written_against_another_schema, "
        "test_rejects_a_capstone_oracle_written_against_another_schema",
    "the capstone oracle describes image {} but the PE oracle describes {}":
        "test_rejects_a_capstone_corpus_built_from_another_image",
    "the inventory pins image {} but the PE oracle describes {}":
        "test_rejects_an_inventory_pinning_another_image",
    "the capstone oracle covers {} executable bytes but the PE oracle records {}":
        "test_rejects_an_executable_extent_the_corpus_disagrees_with",
    "the capstone corpus accounts for {} of {} executable bytes":
        "test_rejects_a_corpus_whose_chunks_and_gaps_do_not_tile_the_extent",
    "Ghidra function {} body overlaps {}":
        "test_rejects_overlapping_function_ownership",
    "translation unit {} occupies {} to {}, which {} interleaves at {}":
        "test_rejects_translation_unit_spans_that_interleave",
    "{} recovered executable targets are unresolved, the first being {}":
        "test_rejects_an_executable_target_no_owned_entry_explains",
    "coverage gap {} publishes {} bytes for an extent of {}":
        "test_rejects_a_gap_byte_string_that_does_not_match_its_extent",
    "the census leaves {} executable bytes unexplained, the first run at {}":
        "test_rejects_a_census_that_leaves_an_executable_byte_unowned",
    "label {} records no {}":
        "test_rejects_a_semantic_label_without_evidence",
    "{} carries a semantic label without a ledger entry":
        "test_rejects_a_semantic_label_the_ledger_does_not_carry",
    "stable ID {} is assigned more than once":
        "test_rejects_duplicate_stable_ids",
    "function {} records no {}":
        "test_rejects_a_row_missing_an_oracle_reference",
    "census passes but {} is {}":
        "test_rejects_a_passing_census_with_unexplained_bytes, "
        "test_rejects_a_passing_census_with_unresolved_targets",
    "{} function entries the evidence names are not owned rows, the first being {}":
        "test_rejects_a_function_entry_no_row_owns",
    "the shape ruling does not resolve against the PE oracle: {}":
        "test_rejects_a_shape_ruling_that_does_not_resolve",
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

        source = (TOOLS_DIR / "reconcile_analysis.py").read_text(encoding="utf-8")
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

    def test_no_census_counter_is_written_as_a_literal(self):
        # Five of the six are measurements; a literal would be an assertion
        # dressed as one, and the guard that catches it has to be mechanical
        # because a passing run drives every one of them to zero.
        import ast

        source = (TOOLS_DIR / "reconcile_analysis.py").read_text(encoding="utf-8")
        counters = {"unexplained_executable_bytes", "unresolved_executable_targets",
                    "unexplained_referenced_data_bytes", "overlaps",
                    "duplicate_ownership"}
        written = {}
        for node in ast.walk(ast.parse(source)):
            if not isinstance(node, ast.Dict):
                continue
            for key, value in zip(node.keys, node.values):
                if isinstance(key, ast.Constant) and key.value in counters:
                    written[key.value] = value
        self.assertEqual(set(written), counters)
        for name, value in sorted(written.items()):
            self.assertNotIsInstance(value, ast.Constant, name)

    def test_every_named_negative_test_exists(self):
        named = {name.strip() for names in RAISE_SITES.values()
                 for name in names.split(",")}
        defined = {name for group in (InputRejectionTests, SelfAuditTests)
                   for name in dir(group) if name.startswith("test_rejects_")}
        self.assertEqual(named, defined)


if __name__ == "__main__":
    unittest.main()
