import json
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import normalize_ghidra  # noqa: E402
import verify_toolchain  # noqa: E402


IMAGE_BASE = 0x10000000

# The pinned analyzer contract lives in verify_toolchain, so the fixture derives
# it rather than restating it; a divergence between the two would otherwise make
# these tests pass against a manifest the pin rejects.
PINNED_ANALYZERS = verify_toolchain.DEFAULT_ANALYSIS_OPTIONS["analyzers"]


def program_record():
    return {
        "tag": "program",
        "name": "NxPhysics.dll",
        "image_base": "0x10000000",
        "language_id": "x86:LE:32:default",
        "compiler_spec_id": "windows",
        "address_space": "ram",
        "ghidra_version": "12.1.2",
        "decompiler_timeout_seconds": 60,
        "decompiler_simplification_style": "decompile",
        "decompiler_options": "DecompileOptions defaults",
        "symbol_table_total": 18153,
        "symbol_scope": "non-dynamic symbols that have an address",
        "analysis_options": dict(PINNED_ANALYZERS),
        # Ghidra's metadata map is emitted whole and carries session-local values
        # alongside the durable ones.
        "metadata": {
            "Created With Ghidra Version": "12.1.2",
            "Executable Format": "Portable Executable (PE)",
            "Executable Location": "C:\\Users\\someone\\project\\NxPhysics.dll",
            "Date Created": "Sun Aug 09 16:34:02 EDT 2026",
            "Program Name": "NxPhysics.dll",
            "Analyzed": "true",
            "SectionAlignment": "4096",
            "# of Functions": "2635",
        },
        "memory_blocks": [
            {
                "name": ".text",
                "start": "0x10001000",
                "end": "0x100010ff",
                "executable": True,
                "initialized": True,
            },
            {
                "name": ".rdata",
                "start": "0x10002000",
                "end": "0x100020ff",
                "executable": False,
                "initialized": True,
            },
        ],
    }


def type_coverage_record():
    return {
        "tag": "type_coverage",
        "header_sha256": "a" * 64,
        "types_parsed": 120,
        "applied_exports": 41,
        "applied_vtables": 37,
        "typed_vtable_slots": 214,
        "skipped_vtable_slots": 3,
        "skipped_slot_reasons": ["0x10001040: function declares no first parameter"],
        "unresolved": [],
        "exports_undeclared_in_headers": ["NxFluidAssert"],
        "non_export_entry_points": ["entry"],
        "excluded_types": [
            {"name": "NxProfiler_DefineZone", "reason": "UNDERIVABLE UNDER THE TARGET ABI"},
        ],
    }


def function_record():
    return {
        "tag": "function",
        "entry": "0x10001000",
        "name": "NxComputeSphereMass",
        "namespace": "",
        "prototype": "float __cdecl NxComputeSphereMass(float radius, float  density)",
        "calling_convention": "__cdecl",
        "stack_purge": 0,
        "thunk": False,
        "thunk_target": None,
        "body": [{"start": "0x10001000", "end": "0x1000101f"}],
        # Deliberately unsorted, and one callee is an import in EXTERNAL space.
        "called": [
            {"address": "0x10001040", "space": "ram"},
            {"address": "0x00000001", "space": "EXTERNAL"},
            {"address": "0x10001020", "space": "ram"},
        ],
        "decompiler_status": "ok",
        # Ghidra's PrettyPrinter ends every line with the host separator; on this
        # host that is CRLF, which must not survive into the manifest.
        "decompiler_c": "float NxComputeSphereMass(float radius,float density)"
                        "\r\n{\r\n  return 0.0;\r\n}\r\n",
        "decompiler_error": None,
    }


def thunk_function_record():
    return {
        "tag": "function",
        "entry": "0x10001020",
        "name": "thunk_NxComputeSphereMass",
        "namespace": "",
        "prototype": "float __cdecl thunk_NxComputeSphereMass(float radius, float density)",
        "calling_convention": "__cdecl",
        "stack_purge": 4,
        "thunk": True,
        "thunk_target": {"address": "0x10001000", "space": "ram"},
        # Two chunks, deliberately emitted out of order.
        "body": [
            {"start": "0x10001060", "end": "0x1000106f"},
            {"start": "0x10001020", "end": "0x10001025"},
        ],
        "called": [],
        "decompiler_status": "failed",
        "decompiler_c": None,
        "decompiler_error": "Low-level Error: Unable to resolve constant",
    }


def valid_records():
    """One record of every tag the exporter emits, deliberately unsorted."""
    return [
        program_record(),
        thunk_function_record(),
        function_record(),
        type_coverage_record(),
        {"tag": "instruction_range", "start": "0x10001040", "end": "0x1000104f", "block": ".text"},
        {"tag": "instruction_range", "start": "0x10001000", "end": "0x1000103f", "block": ".text"},
        {
            "tag": "symbol",
            "address": "0x10001000",
            "space": "ram",
            "name": "NxComputeSphereMass",
            "namespace": "",
            "type": "Function",
            "source": "IMPORTED",
            "primary": True,
            "global": True,
        },
        # An import: Ghidra puts it in the synthetic EXTERNAL space, where the
        # offset is a slot index rather than an address inside the image.
        {
            "tag": "symbol",
            "address": "0x00000001",
            "space": "EXTERNAL",
            "name": "EnterCriticalSection",
            "namespace": "KERNEL32.DLL",
            "type": "Function",
            "source": "IMPORTED",
            "primary": True,
            "global": False,
        },
        {
            "tag": "string",
            "address": "0x10002040",
            "data_type": "string",
            "length": 13,
            "value": "ScannedString",
        },
        {
            "tag": "reference",
            "from": "0x10001010",
            "to": "0x10002040",
            "to_space": "ram",
            "type": "DATA",
            "operand": 0,
            "source": "ANALYSIS",
            "primary": True,
        },
        # A stack reference: its target is an offset in the stack space, not an
        # address the image has.
        {
            "tag": "reference",
            "from": "0x10001004",
            "to": "0x0000000c",
            "to_space": "stack",
            "type": "READ",
            "operand": 1,
            "source": "ANALYSIS",
            "primary": True,
        },
        {
            "tag": "vtable",
            "address": "0x10002000",
            "symbol": "NxShape::vftable",
            "class": "NxShape",
            "slots": ["0x10001000", "0x10001020"],
        },
        {
            "tag": "rtti",
            "address": "0x10002080",
            "kind": "TypeDescriptor",
            "type_name": ".?AVNxShape@@",
            "demangled": "NxShape",
            "symbol": "NxShape::RTTI Type Descriptor",
        },
        {
            "tag": "data",
            "address": "0x10002020",
            "data_type": "NxVec3.conflict1",
            "length": 12,
            "label": "someVector",
        },
    ]


def to_jsonl(records):
    return "".join(json.dumps(record) + "\n" for record in records)


class NormalizeTypeTests(unittest.TestCase):
    def test_collapses_whitespace_runs_into_single_spaces(self):
        self.assertEqual(normalize_ghidra.normalize_type("float   __cdecl  f(int  a)"),
                         "float __cdecl f(int a)")

    def test_strips_ghidra_conflict_suffixes_from_type_names(self):
        self.assertEqual(normalize_ghidra.normalize_type("NxVec3.conflict1 *"), "NxVec3 *")
        self.assertEqual(normalize_ghidra.normalize_type("NxVec3.conflict"), "NxVec3")

    def test_keeps_a_dotted_name_that_is_not_a_conflict_suffix(self):
        self.assertEqual(normalize_ghidra.normalize_type("some.type"), "some.type")

    def test_normalizes_pointer_and_array_spacing(self):
        self.assertEqual(normalize_ghidra.normalize_type("char  *  *"), "char * *")
        self.assertEqual(normalize_ghidra.normalize_type("int [ 4 ]"), "int [4]")


class NormalizeGhidraTests(unittest.TestCase):
    def build(self, records=None):
        return normalize_ghidra.build_manifest(
            normalize_ghidra.parse_records(to_jsonl(records or valid_records())))

    def assertRejects(self, mutate, pattern):
        records = valid_records()
        mutate(records)
        with self.assertRaisesRegex(ValueError, pattern):
            normalize_ghidra.build_manifest(normalize_ghidra.parse_records(to_jsonl(records)))

    def find(self, records, tag):
        return next(record for record in records if record["tag"] == tag)

    def test_records_the_pinned_ghidra_identity_and_analysis_options(self):
        ghidra = self.build()["ghidra"]
        self.assertEqual(ghidra["version"], "12.1.2")
        self.assertEqual(ghidra["language_id"], "x86:LE:32:default")
        self.assertEqual(ghidra["compiler_spec_id"], "windows")
        self.assertEqual(ghidra["analysis_options"], dict(PINNED_ANALYZERS))

    def test_drops_session_local_metadata_and_keeps_the_durable_keys(self):
        metadata = self.build()["ghidra"]["metadata"]
        self.assertEqual(
            metadata,
            {
                "# of Functions": "2635",
                "Created With Ghidra Version": "12.1.2",
                "Executable Format": "Portable Executable (PE)",
                "SectionAlignment": "4096",
            },
        )
        # Each of these names a project path, a wall-clock time, or a session
        # state, so none of them can survive into byte-identical evidence.
        for volatile in ("Executable Location", "Date Created", "Program Name", "Analyzed"):
            self.assertNotIn(volatile, metadata)

    def test_converts_every_address_to_an_rva_relative_to_the_image_base(self):
        manifest = self.build()
        self.assertEqual(manifest["image"], {"name": "NxPhysics.dll", "image_base": "0x10000000"})
        self.assertEqual(manifest["functions"][0]["rva"], "0x00001000")
        self.assertEqual(manifest["strings"][0]["rva"], "0x00002040")
        data_reference = next(entry for entry in manifest["references"]
                              if entry["type"] == "DATA")
        self.assertEqual(data_reference["from_rva"], "0x00001010")
        self.assertEqual(data_reference["to_rva"], "0x00002040")

    def test_orders_functions_by_rva_regardless_of_emission_order(self):
        self.assertEqual(
            [function["rva"] for function in self.build()["functions"]],
            ["0x00001000", "0x00001020"],
        )

    def test_orders_body_chunks_by_rva_and_converts_ends_to_exclusive_sizes(self):
        thunk = self.build()["functions"][1]
        self.assertEqual(
            thunk["body"],
            [
                {"rva": "0x00001020", "end_rva": "0x00001026", "size": 6},
                {"rva": "0x00001060", "end_rva": "0x00001070", "size": 16},
            ],
        )
        self.assertEqual(thunk["body_bytes"], 22)

    def test_orders_call_targets_and_keeps_an_imported_callee_in_its_space(self):
        functions = self.build()["functions"]
        self.assertEqual(
            functions[0]["called"],
            [
                {"rva": "0x00001020", "space": "ram", "space_offset": None},
                {"rva": "0x00001040", "space": "ram", "space_offset": None},
                # An import has no RVA in this image, so it keeps its slot index.
                {"rva": None, "space": "EXTERNAL", "space_offset": 1},
            ],
        )
        self.assertIsNone(functions[0]["thunk_target"])
        self.assertEqual(
            functions[1]["thunk_target"],
            {"rva": "0x00001000", "space": "ram", "space_offset": None},
        )
        self.assertTrue(functions[1]["thunk"])

    def test_normalizes_the_prototype_string_and_keeps_the_calling_convention(self):
        function = self.build()["functions"][0]
        self.assertEqual(
            function["prototype"],
            "float __cdecl NxComputeSphereMass(float radius, float density)",
        )
        self.assertEqual(function["calling_convention"], "__cdecl")
        self.assertEqual(function["stack_purge"], 0)

    def test_keeps_decompiler_output_and_its_status_for_success_and_failure(self):
        functions = self.build()["functions"]
        self.assertEqual(functions[0]["decompiler_status"], "ok")
        self.assertIn("return 0.0;", functions[0]["decompiler_c"])
        self.assertIsNone(functions[0]["decompiler_error"])
        self.assertEqual(functions[1]["decompiler_status"], "failed")
        self.assertIsNone(functions[1]["decompiler_c"])
        self.assertEqual(functions[1]["decompiler_error"], "Low-level Error: Unable to resolve constant")

    def test_orders_and_measures_instruction_ranges(self):
        manifest = self.build()
        self.assertEqual(
            manifest["instruction_ranges"],
            [
                {"rva": "0x00001000", "end_rva": "0x00001040", "size": 64, "block": ".text"},
                {"rva": "0x00001040", "end_rva": "0x00001050", "size": 16, "block": ".text"},
            ],
        )
        self.assertEqual(manifest["coverage"]["instruction_bytes"], 80)

    def test_normalizes_data_type_strings_on_data_records(self):
        self.assertEqual(
            self.build()["data"],
            [
                {
                    "rva": "0x00002020",
                    "data_type": "NxVec3",
                    "length": 12,
                    "label": "someVector",
                }
            ],
        )

    def test_records_vtable_slots_as_rvas_in_slot_order(self):
        self.assertEqual(
            self.build()["vtables"],
            [
                {
                    "rva": "0x00002000",
                    "symbol": "NxShape::vftable",
                    "class": "NxShape",
                    "slots": ["0x00001000", "0x00001020"],
                }
            ],
        )

    def test_records_rtti_and_string_and_symbol_rows(self):
        manifest = self.build()
        self.assertEqual(
            manifest["rtti"],
            [{
                "rva": "0x00002080",
                "kind": "TypeDescriptor",
                "type_name": ".?AVNxShape@@",
                "demangled": "NxShape",
                "symbol": "NxShape::RTTI Type Descriptor",
            }],
        )
        self.assertEqual(manifest["strings"][0]["value"], "ScannedString")
        self.assertEqual(manifest["strings"][0]["data_type"], "string")
        self.assertEqual(manifest["symbols"][0]["name"], "NxComputeSphereMass")
        self.assertEqual(manifest["symbols"][0]["source"], "IMPORTED")

    def test_reports_the_type_coverage_the_typed_pass_measured(self):
        self.assertEqual(
            self.build()["type_coverage"],
            {
                "header_sha256": "a" * 64,
                "types_parsed": 120,
                "applied_exports": 41,
                "applied_vtables": 37,
                # Slots, not tables: a run that typed nothing must not look like
                # one that typed everything.
                "typed_vtable_slots": 214,
                "skipped_vtable_slots": 3,
                # Why a slot went untyped, not just that it did.
                "skipped_slot_reasons": [
                    "0x10001040: function declares no first parameter"],
                "unresolved": [],
                # Exports the public headers never declare are recorded, not hidden:
                # a prototype that cannot exist is not an unresolved type. The
                # image entry point is not an export, so it is kept apart.
                "exports_undeclared_in_headers": ["NxFluidAssert"],
                "non_export_entry_points": ["entry"],
                # A withheld type has to be visible in the evidence, not only in
                # the generator's inputs.
                "excluded_types": [
                    {"name": "NxProfiler_DefineZone",
                     "reason": "UNDERIVABLE UNDER THE TARGET ABI"},
                ],
            },
        )

    def test_keeps_an_external_symbol_with_a_null_rva_and_its_space(self):
        external = [entry for entry in self.build()["symbols"] if entry["rva"] is None]
        self.assertEqual(
            external,
            [{"rva": None, "space": "EXTERNAL", "name": "EnterCriticalSection",
              "namespace": "KERNEL32.DLL", "type": "Function", "source": "IMPORTED",
              "primary": True, "global": False}],
        )

    def test_keeps_a_stack_reference_as_a_space_local_offset(self):
        stack = [entry for entry in self.build()["references"] if entry["to_rva"] is None]
        self.assertEqual(
            stack,
            [{"from_rva": "0x00001004", "to_rva": None, "to_space": "stack",
              "to_space_offset": 12, "type": "READ", "operand": 1,
              "source": "ANALYSIS", "primary": True}],
        )

    def test_counts_every_record_class_it_emitted(self):
        self.assertEqual(
            self.build()["counts"],
            {
                "functions": 2, "instruction_ranges": 2, "symbols": 2, "strings": 1,
                "references": 2, "vtables": 1, "rtti": 1, "data": 1,
                "external_symbols": 1, "non_image_references": 1,
            },
        )

    def test_measures_function_body_coverage_against_executable_blocks(self):
        coverage = self.build()["coverage"]
        self.assertEqual(coverage["function_body_bytes"], 32 + 22)
        self.assertEqual(coverage["executable_bytes"], 256)
        self.assertEqual(coverage["decompiled_functions"], 1)
        self.assertEqual(coverage["failed_functions"], 1)

    def test_normalizing_the_same_stream_twice_produces_identical_bytes(self):
        first = normalize_ghidra.serialize(self.build())
        shuffled = valid_records()
        shuffled.reverse()
        second = normalize_ghidra.serialize(self.build(shuffled))
        self.assertEqual(first, second)

    def test_rejects_a_line_that_is_not_valid_json(self):
        with self.assertRaisesRegex(ValueError, "line 2 is not valid JSON"):
            normalize_ghidra.parse_records(
                json.dumps(program_record()) + "\n{not json}\n")

    def test_rejects_a_record_without_a_tag(self):
        self.assertRejects(
            lambda records: records[4].pop("tag"),
            "line 5 has no tag",
        )

    def test_rejects_a_record_with_an_unknown_tag(self):
        self.assertRejects(
            lambda records: records[4].__setitem__("tag", "guess"),
            "line 5 has unknown tag 'guess'",
        )

    def test_rejects_a_record_missing_a_required_field(self):
        self.assertRejects(
            lambda records: records[2].pop("stack_purge"),
            "line 3: function record is missing 'stack_purge'",
        )

    def test_rejects_a_record_carrying_an_unexpected_field(self):
        self.assertRejects(
            lambda records: records[2].__setitem__("analysis_seconds", 12.5),
            "line 3: function record has unexpected field 'analysis_seconds'",
        )

    def test_rejects_a_stream_without_a_program_record(self):
        self.assertRejects(
            lambda records: records.pop(0),
            "the stream has 0 program records",
        )

    def test_rejects_a_stream_without_a_type_coverage_record(self):
        # Only the typed second pass emits type coverage, so a stream without it
        # is the untyped first pass and must not become evidence.
        self.assertRejects(
            lambda records: records.pop(3),
            "the stream has 0 type_coverage records",
        )

    def test_rejects_a_stream_carrying_two_program_records(self):
        self.assertRejects(
            lambda records: records.append(program_record()),
            "the stream has 2 program records",
        )

    def test_rejects_an_address_below_the_image_base(self):
        self.assertRejects(
            lambda records: records[2].__setitem__("entry", "0x0fff0000"),
            "address 0x0fff0000 is below image base 0x10000000",
        )

    def test_rejects_an_address_that_is_not_hexadecimal(self):
        self.assertRejects(
            lambda records: records[2].__setitem__("entry", "ptr_0001"),
            "'ptr_0001' is not a hexadecimal address",
        )

    def test_rejects_a_body_range_that_ends_before_it_starts(self):
        self.assertRejects(
            lambda records: records[2]["body"][0].__setitem__("end", "0x10000ff0"),
            "function 0x00001000 has body range 0x00001000-0x00000ff0 ending before it starts",
        )

    def test_rejects_two_functions_sharing_one_entry_point(self):
        self.assertRejects(
            lambda records: records[1].__setitem__("entry", "0x10001000"),
            "two functions share entry 0x00001000",
        )

    def test_rejects_type_coverage_that_left_a_public_type_unresolved(self):
        self.assertRejects(
            lambda records: records[3].__setitem__("unresolved", ["NxSphericalJoint"]),
            "type coverage left public types unresolved: NxSphericalJoint",
        )

    def test_rejects_an_analysis_option_that_disagrees_with_the_pin(self):
        self.assertRejects(
            lambda records: records[0]["analysis_options"].__setitem__(
                "Decompiler Parameter ID", False),
            "analysis option 'Decompiler Parameter ID' is False but the pin requires True",
        )

    def test_rejects_an_analysis_option_the_pin_does_not_name(self):
        self.assertRejects(
            lambda records: records[0]["analysis_options"].pop("Demangler Microsoft"),
            "analysis options do not name the pinned analyzer 'Demangler Microsoft'",
        )

    def test_normalizes_the_host_line_separator_out_of_decompiler_output(self):
        # Ghidra's PrettyPrinter appends System.getProperty("line.separator"), so
        # a manifest that kept it would hash differently on Windows and Linux.
        decompiled = self.build()["functions"][0]["decompiler_c"]
        self.assertNotIn("\r", decompiled)
        self.assertEqual(
            decompiled,
            "float NxComputeSphereMass(float radius,float density)\n{\n  return 0.0;\n}\n",
        )

    def test_records_the_decompiler_settings_that_decide_the_status(self):
        self.assertEqual(
            self.build()["decompiler"],
            {"timeout_seconds": 60, "simplification_style": "decompile",
             "options": "DecompileOptions defaults"},
        )

    def test_records_how_filtered_the_symbol_view_is(self):
        self.assertEqual(
            self.build()["symbol_scope"],
            {"description": "non-dynamic symbols that have an address",
             "symbol_table_total": 18153},
        )

    def test_keeps_the_stable_counters_and_the_section_alignment(self):
        metadata = self.build()["ghidra"]["metadata"]
        # SectionAlignment explains the executable-extent difference against the
        # PE oracle, and the counters let a reader detect a filtered table.
        self.assertEqual(metadata["SectionAlignment"], "4096")
        self.assertEqual(metadata["# of Functions"], "2635")

    def test_rejects_a_stream_whose_ghidra_version_is_not_the_pinned_one(self):
        self.assertRejects(
            lambda records: records[0].__setitem__("ghidra_version", "11.4.2"),
            "ghidra.version is '11.4.2' but the pin records '12.1.2'",
        )

    def test_rejects_a_stream_whose_language_is_not_the_pinned_one(self):
        self.assertRejects(
            lambda records: records[0].__setitem__("language_id", "x86:LE:64:default"),
            "language_id is 'x86:LE:64:default' but the pin records 'x86:LE:32:default'",
        )

    def test_rejects_an_analyzer_the_pin_does_not_name_at_all(self):
        # The field checks compare the six pinned analyzers name by name, so the
        # options hash is the only thing that can catch a seventh analyzer having
        # been in force. That is this branch, and it is the reason the hash
        # comparison earns its place.
        records = valid_records()
        records[0]["analysis_options"]["Stack"] = True
        with self.assertRaisesRegex(ValueError, "analysis-options hash is [0-9a-f]{64} "
                                                "but the pin records"):
            normalize_ghidra.build_manifest(normalize_ghidra.parse_records(to_jsonl(records)))

    def test_rejects_an_unreadable_toolchain_pin(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, "cannot read the toolchain pin"):
                normalize_ghidra.load_pin(Path(directory) / "missing.json")

    def test_accepts_an_explicitly_supplied_pin(self):
        # build_manifest takes a pin so a caller can verify against one that is
        # not the tool's own neighbour; the CLI uses the default.
        manifest = normalize_ghidra.build_manifest(
            normalize_ghidra.parse_records(to_jsonl(valid_records())),
            pin=normalize_ghidra.load_pin())
        self.assertEqual(manifest["ghidra"]["version"], "12.1.2")

    def test_rejects_a_stream_whose_compiler_spec_is_not_the_pinned_one(self):
        self.assertRejects(
            lambda records: records[0].__setitem__("compiler_spec_id", "gcc"),
            "compiler_spec_id is 'gcc' but the pin records 'windows'",
        )

    def test_rejects_a_decompiler_status_the_schema_does_not_define(self):
        self.assertRejects(
            lambda records: records[2].__setitem__("decompiler_status", "partial"),
            "function 0x00001000 has unknown decompiler status 'partial'",
        )

    def test_rejects_a_function_body_outside_every_memory_block(self):
        self.assertRejects(
            lambda records: records[2]["body"][0].update(
                {"start": "0x10009000", "end": "0x1000900f"}),
            "function 0x00001000 body range 0x00009000-0x00009010 is outside every memory block",
        )


class PinnedAnalyzerContractTests(unittest.TestCase):
    """The exporter restates the analyzer contract in Java, where nothing else
    would notice it drifting from analysis_toolchain.json."""

    def test_the_exporters_analyzer_table_matches_the_pin(self):
        source = (TOOLS_DIR.parent / "ghidra" / "ExportPhysicsAnalysis.java").read_text(
            encoding="utf-8")
        block = re.search(
            r"PINNED_ANALYZERS = \{(.*?)\};", source, re.DOTALL).group(1)
        declared = {
            name: value == "true"
            for name, value in re.findall(r'\{\s*"([^"]+)",\s*"(true|false)"\s*\}', block)
        }
        self.assertEqual(declared, dict(PINNED_ANALYZERS))

    def test_the_recorded_pin_matches_the_verifier_contract(self):
        pin = normalize_ghidra.load_pin()
        self.assertEqual(pin["analysis_options"]["analyzers"], dict(PINNED_ANALYZERS))
        self.assertEqual(
            pin["analysis_options_sha256"],
            verify_toolchain.canonical_options_sha256(pin["analysis_options"]),
        )


class NormalizeGhidraCliTests(unittest.TestCase):
    def run_cli(self, text, extra=()):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "ghidra.raw.jsonl"
            source.write_text(text, encoding="utf-8")
            output = root / "oracle" / "ghidra" / "manifest.json"
            result = subprocess.run(
                [sys.executable, str(TOOLS_DIR / "normalize_ghidra.py"),
                 "--input", str(source), "--output", str(output), *extra],
                capture_output=True, text=True, check=False,
            )
            content = output.read_bytes() if output.is_file() else None
            return result, content

    def test_writes_the_manifest_and_reports_its_measurements(self):
        result, content = self.run_cli(to_jsonl(valid_records()))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("ghidra=written", result.stdout)
        self.assertIn("functions=2", result.stdout)
        self.assertIn("decompiled=1", result.stdout)
        self.assertIn("failed=1", result.stdout)
        self.assertIn("vtables=1", result.stdout)
        self.assertIn("rtti=1", result.stdout)
        self.assertIn("strings=1", result.stdout)
        manifest = json.loads(content.decode("utf-8"))
        self.assertEqual(manifest["schema_version"], normalize_ghidra.SCHEMA_VERSION)

    def test_writes_byte_identical_output_for_a_reordered_stream(self):
        _, first = self.run_cli(to_jsonl(valid_records()))
        _, second = self.run_cli(to_jsonl(list(reversed(valid_records()))))
        self.assertEqual(first, second)

    def test_unreadable_input_reports_cli_error_without_traceback(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run(
                [sys.executable, str(TOOLS_DIR / "normalize_ghidra.py"),
                 "--input", str(Path(directory) / "missing.jsonl"),
                 "--output", str(Path(directory) / "manifest.json")],
                capture_output=True, text=True, check=False,
            )
        self.assertEqual(result.returncode, 2)
        self.assertIn("error:", result.stderr)
        self.assertNotIn("Traceback", result.stderr)

    def test_a_rejected_stream_reports_the_reason_without_traceback(self):
        broken = valid_records()
        broken[2]["entry"] = "0x0fff0000"
        result, content = self.run_cli(to_jsonl(broken))
        self.assertEqual(result.returncode, 2)
        self.assertIn("is below image base", result.stderr)
        self.assertNotIn("Traceback", result.stderr)
        self.assertIsNone(content)


if __name__ == "__main__":
    unittest.main()
