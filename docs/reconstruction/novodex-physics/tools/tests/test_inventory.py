import copy
import csv
import json
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import validate_inventory  # noqa: E402


def minimal_inventory():
    return {
        "schema_version": 1,
        "pins": {
            "oracle": {
                "path": "Binaries/NxPhysics.dll",
                "sha256": "4b" + "0" * 62,
                "size": 1253376,
                "named_exports": 1,
            },
            "link_oracle": {
                "path": "Development/External/Novodex/Physics/lib/win32/Release/NxPhysics.lib",
                "sha256": "a9" + "0" * 62,
            },
            "public_headers": {
                "root": "Development/External/Novodex/",
                "sdk_version": "2.1.2.6000",
                "physics_manifest": "public_header_hashes.json",
                "foundation_manifest": "../novodex-foundation/dumps/immutable_public_hashes.json",
            },
            "analysis_toolchain": "analysis_toolchain.json",
            "labels": "labels.json",
        },
        "sections": [
            {
                "name": ".text",
                "rva": "0x00001000",
                "virtual_size": 4096,
                "raw_offset": 1024,
                "raw_size": 4096,
                "characteristics": "0x60000020",
                "executable": True,
            }
        ],
        "functions": [
            {
                "id": "phys_fn_000001",
                "rva": "0x00001000",
                "size": 16,
                "kind": "code",
                "label": "phys_fn_000001",
                "label_confidence": "stable-id",
                "section": ".text",
                "phase": 2,
                "phase_provenance": "translation_unit",
                "state": "discovered",
                "source": None,
                "ghidra_ref": "ghidra/functions/phys_fn_000001.json",
                "capstone_ref": "capstone/phys_fn_000001.json",
                "static_proof": None,
                "dynamic_proof": None,
                "notes": "",
            },
            {
                "id": "phys_fn_000002",
                "rva": "0x00001010",
                "size": 32,
                "kind": "code",
                "label": "NxCreatePhysicsSDK",
                "label_confidence": "semantic",
                "section": ".text",
                "phase": 2,
                "phase_provenance": "export_pin",
                "state": "typed",
                "source": None,
                "ghidra_ref": "ghidra/functions/phys_fn_000002.json",
                "capstone_ref": "capstone/phys_fn_000002.json",
                "static_proof": None,
                "dynamic_proof": None,
                "notes": "",
            },
        ],
        "data_objects": [
            {
                "id": "phys_data_000001",
                "rva": "0x00003000",
                "size": 8,
                "type": "string",
                "owner": "phys_fn_000002",
                "references": ["0x00001010"],
                "section": ".rdata",
                "phase": 2,
                "phase_provenance": "reading_sites",
                "state": "classified",
                "source": None,
                "label": "phys_data_000001",
                "label_confidence": "stable-id",
                "structural_proof":
                    "a NUL-terminated printable run the PE string scan recorded",
                "notes": "",
            }
        ],
        "exports": [
            {
                "ordinal": 1,
                "name": "NxCreatePhysicsSDK",
                "rva": "0x00001010",
                "function_id": "phys_fn_000002",
            }
        ],
        "imports": [
            {
                "dll": "NxFoundation.dll",
                "name": "?getInstance@FoundationSDK@NxFoundation@@SAAAV12@XZ",
                "ordinal": None,
                "iat_rva": "0x00005000",
            }
        ],
        "coverage": {
            "census": {
                "status": "pending",
                "code_report": "oracle/coverage.json",
                "data_report": "oracle/data-coverage.json",
            },
            "executable_bytes": 0,
            "explained_executable_bytes": 0,
            "unexplained_executable_bytes": 0,
            "unresolved_executable_targets": 0,
            "referenced_data_bytes": 0,
            "unexplained_referenced_data_bytes": 0,
            "overlaps": 0,
            "duplicate_ownership": 0,
        },
        "phases": [
            {"phase": 1, "name": "oracle census", "status": "pending"},
            {"phase": 2, "name": "shared runtime", "status": "pending"},
        ],
        "gates": {
            "toolchain_pinned": "pending",
            "public_headers_pinned": "pending",
            "pe_manifest": "pending",
            "ghidra_semantics": "pending",
            "capstone_corpus": "pending",
            "phase_1_oracle_census": "pending",
        },
    }


def passing_inventory():
    """A minimal inventory whose census legitimately passes: non-zero totals,
    fully explained bytes, and an export table matching the pinned count."""
    data = minimal_inventory()
    data["coverage"]["census"]["status"] = "pass"
    data["coverage"]["executable_bytes"] = 1253376
    data["coverage"]["explained_executable_bytes"] = 1253376
    data["coverage"]["referenced_data_bytes"] = 4096
    return data


def minimal_program():
    """The programme record that quotes the minimal inventory back at itself."""
    pins = minimal_inventory()["pins"]
    return {
        "schema_version": 1,
        "oracle": dict(pins["oracle"]),
        "link_oracle": dict(pins["link_oracle"]),
        "header_root": dict(pins["public_headers"]),
        "foundation_commit": "6" * 40,
        "evidence_commit": "b" * 40,
        "phases": [
            {
                "phase": 1,
                "name": "oracle census",
                "plan": "docs/superpowers/plans/phase1.md",
                "gate": "every executable byte classified",
                "status": "pending",
                "implementation_commit": None,
                "evidence_commit": None,
                "gate_artifact": None,
                "owned_functions": 0,
                "owned_data_objects": 0,
                "closed_functions": None,
                "closed_data_objects": None,
                "remaining_functions": None,
                "remaining_data_objects": None,
            },
            {
                "phase": 2,
                "name": "shared runtime",
                "plan": "docs/superpowers/plans/phase2.md",
                "gate": "the shared runtime closes",
                "status": "pending",
                "implementation_commit": None,
                "evidence_commit": None,
                "gate_artifact": None,
                "owned_functions": 2,
                "owned_data_objects": 1,
                "closed_functions": None,
                "closed_data_objects": None,
                "remaining_functions": None,
                "remaining_data_objects": None,
            },
        ],
        "global_gates": [
            {"name": "build_configure", "command": "cmake --fresh", "status": "pending"},
        ],
    }


def minimal_labels():
    return {
        "schema_version": 1,
        "labels": [
            {
                "id": "phys_fn_000002",
                "label": "NxCreatePhysicsSDK",
                "confidence": "semantic",
                "evidence": "oracle/pe.json#exports/NxCreatePhysicsSDK",
                "reason": "named PE export at the function entry RVA",
            }
        ],
    }


# The correspondence maps the third-party column is checked against. Written
# into the temporary evidence tree so the CLI reads real files: a check that
# passes because its input is absent is the failure mode this whole gate exists
# to avoid, so the absent case is a test of its own rather than the default.
MAP_COLUMNS = ("rva", "id", "size", "grade", "source_file", "source_line",
               "source_function", "signals", "notes")
MINIMAL_MAPS = {
    "qhull_map.csv": [],
    "opcode_map.csv": [],
    "opcode_outside_span_map.csv": [],
}


def map_row(identifier, rva, grade="mapped", source_file="qhull.c",
            source_function="qh_qhull"):
    return {"rva": rva, "id": identifier, "size": "16", "grade": grade,
            "source_file": source_file, "source_line": "1",
            "source_function": source_function, "signals": "S1-string", "notes": ""}


def write_correspondence_maps(root, maps):
    directory = root / validate_inventory.THIRD_PARTY_MAP_DIR
    directory.mkdir(parents=True, exist_ok=True)
    for name, rows in maps.items():
        with (directory / name).open("w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=MAP_COLUMNS, lineterminator="\n")
            writer.writeheader()
            for row in rows:
                writer.writerow(row)


# The pinned upstream trees a correspondence has to resolve against, for the
# same reason the maps themselves are written out: a correspondence checked
# against a tree that is not there is not checked.
PINNED_SOURCES = {
    "qhull-2003.1/src/qhull.c": "void qh_qhull(void) {}\n",
    "opcode13/Opcode/OPC_Model.cpp": "Model::~Model() {}\n",
    "opcode13/Opcode/Ice/IceContainer.cpp": "Container::Container() {}\n",
}


def write_pinned_sources(root, sources=PINNED_SOURCES):
    for name, text in sources.items():
        path = root / validate_inventory.THIRD_PARTY_SOURCE_DIR / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")


class ValidateInventoryTests(unittest.TestCase):
    def assertRejects(self, data, pattern):
        errors = validate_inventory.validate_inventory(data)
        self.assertTrue(errors, "expected at least one error")
        self.assertRegex("\n".join(errors), pattern)

    def test_minimal_fixture_is_valid(self):
        self.assertEqual(validate_inventory.validate_inventory(minimal_inventory()), [])

    def test_rejects_missing_top_level_key(self):
        data = minimal_inventory()
        del data["coverage"]
        self.assertRejects(data, "missing key 'coverage'")

    def test_rejects_unexpected_top_level_key(self):
        data = minimal_inventory()
        data["clusters"] = []
        self.assertRejects(data, "unexpected key 'clusters'")

    def test_rejects_unexpected_function_field(self):
        data = minimal_inventory()
        data["functions"][0]["confidence"] = "high"
        self.assertRejects(data, r"functions\[0\] has unexpected key 'confidence'")

    def test_accepts_the_optional_third_party_field(self):
        data = minimal_inventory()
        data["functions"][0]["third_party"] = "qhull"
        self.assertEqual(validate_inventory.validate_inventory(data), [])

    def test_a_data_object_may_not_carry_the_third_party_field(self):
        # There is no per-object correspondence map, so the column would be an
        # unfalsifiable claim on the data half of the census.
        data = minimal_inventory()
        data["data_objects"][0]["third_party"] = "qhull"
        self.assertRejects(data, r"data_objects\[0\] has unexpected key 'third_party'")

    def test_rejects_duplicate_function_ids(self):
        data = minimal_inventory()
        data["functions"][1]["id"] = "phys_fn_000001"
        self.assertRejects(data, "stable ID 'phys_fn_000001' is used more than once")

    def test_rejects_stable_id_shared_between_function_and_data(self):
        data = minimal_inventory()
        data["data_objects"][0]["id"] = "phys_fn_000001"
        self.assertRejects(data, "stable ID 'phys_fn_000001' is used more than once")

    def test_rejects_non_conforming_stable_id(self):
        data = minimal_inventory()
        data["functions"][0]["id"] = "sub_401000"
        self.assertRejects(data, "not a phys_fn")

    def test_rejects_overlapping_code_ranges(self):
        data = minimal_inventory()
        data["functions"][0]["size"] = 32
        self.assertRejects(data, "overlaps")

    def test_rejects_a_data_object_overlapping_its_neighbour(self):
        data = minimal_inventory()
        data["data_objects"].append(dict(data["data_objects"][0],
                                         id="phys_data_000002", rva="0x00003004"))
        self.assertRejects(data, "overlaps")

    def test_rejects_a_data_object_overlapping_a_function(self):
        # A .text data object that runs into a code row is the one invariant the
        # whole census exists to establish, and later phases re-run only this.
        data = minimal_inventory()
        data["data_objects"][0]["rva"] = "0x00001008"
        data["data_objects"][0]["section"] = ".text"
        self.assertRejects(data, "overlaps")

    def test_reports_overlaps_alongside_unrelated_errors(self):
        data = minimal_inventory()
        data["functions"][0]["size"] = 32
        data["functions"][1]["state"] = "in_progress"
        errors = "\n".join(validate_inventory.validate_inventory(data))
        self.assertRegex(errors, "overlaps")
        self.assertRegex(errors, "in_progress")

    def test_malformed_stable_id_is_reported_without_crashing(self):
        data = minimal_inventory()
        data["functions"][0]["id"] = ["phys_fn_000001"]
        self.assertRejects(data, "must be a string")

    def test_accepts_adjacent_code_ranges(self):
        data = minimal_inventory()
        data["functions"][0]["size"] = 16
        self.assertEqual(validate_inventory.validate_inventory(data), [])

    def test_rejects_unknown_state(self):
        data = minimal_inventory()
        data["functions"][0]["state"] = "in_progress"
        self.assertRejects(data, "state 'in_progress'")

    def test_rejects_unknown_kind(self):
        data = minimal_inventory()
        data["functions"][0]["kind"] = "data"
        self.assertRejects(data, "kind 'data'")

    def test_rejects_missing_phase_owner(self):
        data = minimal_inventory()
        data["functions"][0]["phase"] = None
        self.assertRejects(data, "exactly one phase owner")

    def test_rejects_undeclared_phase(self):
        data = minimal_inventory()
        data["functions"][0]["phase"] = 9
        self.assertRejects(data, "undeclared phase 9")

    def test_rejects_data_object_without_phase_owner(self):
        data = minimal_inventory()
        data["data_objects"][0]["phase"] = None
        self.assertRejects(data, "exactly one phase owner")

    def test_rejects_a_phase_decided_by_no_named_rule(self):
        # A phase without the rule that decided it is the state this census
        # left Phase 2 in: a number a later phase cannot audit.
        data = minimal_inventory()
        data["functions"][0]["phase_provenance"] = "seemed_right"
        self.assertRejects(data, "phase_provenance 'seemed_right'")

    def test_rejects_a_data_object_phase_decided_by_no_named_rule(self):
        data = minimal_inventory()
        data["data_objects"][0]["phase_provenance"] = "shared_default"
        self.assertRejects(data, "phase_provenance 'shared_default'")

    def test_rejects_code_row_without_ghidra_ref(self):
        data = minimal_inventory()
        data["functions"][0]["ghidra_ref"] = None
        self.assertRejects(data, "must record ghidra_ref")

    def test_rejects_code_row_without_capstone_ref(self):
        data = minimal_inventory()
        data["functions"][0]["capstone_ref"] = None
        self.assertRejects(data, "must record capstone_ref")

    def test_rejects_compiler_artifact_without_classification_proof(self):
        data = minimal_inventory()
        data["functions"][0]["kind"] = "compiler_artifact"
        self.assertRejects(data, "classification proof")

    def test_rejects_compiler_artifact_claiming_product_source(self):
        data = minimal_inventory()
        data["functions"][0]["kind"] = "compiler_artifact"
        data["functions"][0]["static_proof"] = "capstone/crt_stub.json"
        data["functions"][0]["source"] = "Physics/src/NxScene.cpp"
        self.assertRejects(data, "must not claim product source")

    def test_accepts_proven_compiler_artifact(self):
        data = minimal_inventory()
        data["functions"][0]["kind"] = "compiler_artifact"
        data["functions"][0]["static_proof"] = "capstone/crt_stub.json"
        # An artifact's terminal state is `classified`: it has no behaviour to mutate, so it is
        # not reached through the closure a code row uses.
        data["functions"][0]["state"] = "classified"
        self.assertEqual(validate_inventory.validate_inventory(data), [])

    def test_rejects_artifact_left_at_a_non_terminal_state(self):
        """Without this half an artifact could sit at `discovered` forever."""
        data = minimal_inventory()
        data["functions"][0]["kind"] = "compiler_artifact"
        data["functions"][0]["static_proof"] = "capstone/crt_stub.json"
        data["functions"][0]["state"] = "discovered"
        self.assertRejects(data, "its terminal state is 'classified'")

    def test_rejects_code_row_claiming_classified(self):
        """Without this half a code row could escape the closure requirement."""
        data = minimal_inventory()
        data["functions"][0]["state"] = "classified"
        self.assertRejects(data, "which is true only of a compiler_artifact")

    def test_classified_ranks_level_with_closed(self):
        """They are two terminal states, not one below the other."""
        self.assertEqual(validate_inventory.STATE_RANK["classified"],
                         validate_inventory.STATE_RANK["closed"])

    def test_accepts_classified_artifact_without_a_closure_ledger(self):
        """An artifact's evidence is its classification proof, not a mutation a gate caught."""
        data = minimal_inventory()
        data["functions"][0]["kind"] = "compiler_artifact"
        data["functions"][0]["static_proof"] = "capstone/crt_stub.json"
        data["functions"][0]["state"] = "classified"
        closures = {1: {"schema_version": 1, "phase": 1, "counts": {"closed": 0, "deferred": []},
                        "closed": [], "deferred": [
                            {"id": data["functions"][0]["id"], "reason": "not_reconstructed_in_phase",
                             "phase_provenance": "pe_structure", "driving_phases": []}]}}
        self.assertEqual(validate_inventory.validate_row_states(data, closures), [])

    def test_accepts_a_data_object_whose_proof_matches_its_type(self):
        data = minimal_inventory()
        data["data_objects"][0]["type"] = "string"
        data["data_objects"][0]["structural_proof"] = (
            "a NUL-terminated printable run the PE string scan recorded")
        self.assertEqual(validate_inventory.validate_inventory(data), [])

    def test_rejects_a_data_object_type_outside_the_vocabulary(self):
        """The type determines the proof, so an unknown type determines nothing."""
        data = minimal_inventory()
        data["data_objects"][0]["type"] = "whatever"
        self.assertRejects(data, "which is not one of")

    def test_rejects_a_data_object_proof_its_type_does_not_determine(self):
        """A value check, not a presence check: the proof must be the one the type gives."""
        data = minimal_inventory()
        data["data_objects"][0]["type"] = "string"
        data["data_objects"][0]["structural_proof"] = "because I said so"
        self.assertRejects(data, "but the row records")

    def test_rejects_a_templated_proof_whose_parameter_is_malformed(self):
        """The one templated family: its address is checked for shape."""
        data = minimal_inventory()
        data["data_objects"][0]["type"] = "switch_table"
        data["data_objects"][0]["structural_proof"] = (
            "a decoded jmp at 0xNOTHEX names this table and the PE oracle relocates every "
            "slot it walks")
        self.assertRejects(data, "whose structural proof is the template")

    def test_accepts_a_templated_proof_with_a_well_formed_parameter(self):
        data = minimal_inventory()
        data["data_objects"][0]["type"] = "switch_table"
        data["data_objects"][0]["structural_proof"] = (
            "a decoded jmp at 0x00001e2c names this table and the PE oracle relocates every "
            "slot it walks")
        self.assertEqual(validate_inventory.validate_inventory(data), [])

    def test_the_data_vocabulary_is_closed_over_every_type(self):
        """Every type the census can carry has a proof rule, and the two sets agree."""
        types = (set(validate_inventory.DATA_PROOF_BY_TYPE)
                 | set(validate_inventory.DATA_PROOF_ALTERNATIVES)
                 | set(validate_inventory.DATA_PROOF_TEMPLATE))
        self.assertEqual(types, set(validate_inventory.DATA_TYPES))
        self.assertEqual(len(types), len(validate_inventory.DATA_TYPES))

    def test_accepts_a_classified_data_object(self):
        data = minimal_inventory()
        data["data_objects"][0]["state"] = "classified"
        self.assertEqual(validate_inventory.validate_inventory(data), [])

    def test_rejects_a_data_object_left_below_the_terminal_rung(self):
        """A data object has no behaviour to mutate, so it never passes through the code ladder."""
        data = minimal_inventory()
        data["data_objects"][0]["state"] = "discovered"
        self.assertRejects(data, "its terminal state is 'classified'")

    def test_accepts_a_classified_data_object_without_a_closure_ledger(self):
        """Its evidence is the structural proof, not a mutation a gate caught."""
        data = minimal_inventory()
        data["data_objects"][0]["state"] = "classified"
        closures = {1: {"schema_version": 1, "phase": 1, "counts": {"closed": 0, "deferred": []},
                        "closed": [], "deferred": [
                            {"id": data["data_objects"][0]["id"],
                             "reason": "data_object_not_dispositioned",
                             "phase_provenance": "pe_structure", "driving_phases": []}]}}
        self.assertEqual(validate_inventory.validate_row_states(data, closures), [])

    def test_rejects_duplicate_export_ownership(self):
        data = minimal_inventory()
        data["exports"].append(copy.deepcopy(data["exports"][0]))
        self.assertRejects(data, "'NxCreatePhysicsSDK' is claimed more than once")

    def test_rejects_export_count_disagreeing_with_the_named_export_pin(self):
        data = minimal_inventory()
        data["pins"]["oracle"]["named_exports"] = 41
        self.assertRejects(
            data, r"exports records 1 named exports but pins\.oracle\.named_exports pins 41"
        )

    def test_empty_export_table_does_not_trip_the_named_export_pin(self):
        data = minimal_inventory()
        data["exports"] = []
        data["pins"]["oracle"]["named_exports"] = 41
        self.assertEqual(validate_inventory.validate_inventory(data), [])

    def test_rejects_export_owned_by_unknown_function(self):
        data = minimal_inventory()
        data["exports"][0]["function_id"] = "phys_fn_000404"
        self.assertRejects(data, "unknown owner 'phys_fn_000404'")

    def test_rejects_passing_census_with_unexplained_executable_bytes(self):
        data = passing_inventory()
        data["coverage"]["explained_executable_bytes"] = 1253364
        data["coverage"]["unexplained_executable_bytes"] = 12
        self.assertRejects(data, r"census passes but coverage\.unexplained_executable_bytes is 12")

    def test_rejects_passing_census_with_unexplained_data_bytes(self):
        data = passing_inventory()
        data["coverage"]["unexplained_referenced_data_bytes"] = 4
        self.assertRejects(data, r"coverage\.unexplained_referenced_data_bytes is 4")

    def test_rejects_executable_coverage_totals_that_do_not_add_up(self):
        data = minimal_inventory()
        data["coverage"]["executable_bytes"] = 1253376
        self.assertRejects(data, r"must equal coverage\.executable_bytes \(1253376\)")

    def test_rejects_a_passing_census_that_leaves_executable_bytes_unaccounted(self):
        data = passing_inventory()
        data["coverage"]["explained_executable_bytes"] = 0
        self.assertRejects(data, r"must equal coverage\.executable_bytes")

    def test_rejects_passing_census_with_zero_executable_bytes(self):
        data = passing_inventory()
        data["coverage"]["executable_bytes"] = 0
        data["coverage"]["explained_executable_bytes"] = 0
        self.assertRejects(data, r"census passes but coverage\.executable_bytes is 0")

    def test_rejects_passing_census_with_zero_referenced_data_bytes(self):
        data = passing_inventory()
        data["coverage"]["referenced_data_bytes"] = 0
        self.assertRejects(data, r"census passes but coverage\.referenced_data_bytes is 0")

    def test_rejects_passing_census_with_an_empty_export_table(self):
        data = passing_inventory()
        data["exports"] = []
        self.assertRejects(
            data, r"exports records 0 named exports but pins\.oracle\.named_exports pins 1"
        )

    def test_rejects_an_entirely_empty_passing_census(self):
        data = minimal_inventory()
        data["coverage"]["census"]["status"] = "pass"
        data["exports"] = []
        errors = "\n".join(validate_inventory.validate_inventory(data))
        self.assertRegex(errors, r"census passes but coverage\.executable_bytes is 0")
        self.assertRegex(errors, r"census passes but coverage\.referenced_data_bytes is 0")
        self.assertRegex(errors, r"exports records 0 named exports")

    def test_rejects_unexplained_data_bytes_exceeding_referenced_data_bytes(self):
        data = minimal_inventory()
        data["coverage"]["unexplained_referenced_data_bytes"] = 8
        self.assertRejects(
            data,
            r"coverage\.unexplained_referenced_data_bytes \(8\) cannot exceed "
            r"coverage\.referenced_data_bytes \(0\)",
        )

    def test_accepts_unexplained_data_bytes_within_referenced_data_bytes(self):
        data = minimal_inventory()
        data["coverage"]["referenced_data_bytes"] = 4096
        data["coverage"]["unexplained_referenced_data_bytes"] = 4096
        self.assertEqual(validate_inventory.validate_inventory(data), [])

    def test_accepts_passing_census_with_zero_unexplained_bytes(self):
        self.assertEqual(validate_inventory.validate_inventory(passing_inventory()), [])

    def test_accepts_phase_one_gate_passing_behind_a_real_census(self):
        data = passing_inventory()
        data["gates"]["phase_1_oracle_census"] = "pass"
        self.assertEqual(validate_inventory.validate_inventory(data), [])

    def test_rejects_phase_one_gate_passing_ahead_of_the_census(self):
        data = minimal_inventory()
        data["gates"]["phase_1_oracle_census"] = "pass"
        self.assertRejects(data, "gate 'phase_1_oracle_census' passes but the census")

    def test_rejects_unknown_gate_status(self):
        data = minimal_inventory()
        data["gates"]["pe_manifest"] = "done"
        self.assertRejects(data, "gate 'pe_manifest'")


class ValidateLabelsTests(unittest.TestCase):
    def assertRejects(self, labels, pattern, inventory=None):
        errors = validate_inventory.validate_labels(inventory or minimal_inventory(), labels)
        self.assertTrue(errors, "expected at least one error")
        self.assertRegex("\n".join(errors), pattern)

    def test_minimal_ledger_is_valid(self):
        self.assertEqual(
            validate_inventory.validate_labels(minimal_inventory(), minimal_labels()), []
        )

    def test_rejects_label_for_unknown_stable_id(self):
        labels = minimal_labels()
        labels["labels"][0]["id"] = "phys_fn_000404"
        self.assertRejects(labels, "unknown stable ID 'phys_fn_000404'")

    def test_malformed_ledger_id_is_reported_without_crashing(self):
        labels = minimal_labels()
        labels["labels"][0]["id"] = ["phys_fn_000002"]
        self.assertRejects(labels, "label ledger id .* must be a string")

    def test_rejects_duplicate_ledger_entries(self):
        labels = minimal_labels()
        labels["labels"].append(copy.deepcopy(labels["labels"][0]))
        self.assertRejects(labels, "more than once")

    def test_rejects_ledger_label_disagreeing_with_the_inventory(self):
        labels = minimal_labels()
        labels["labels"][0]["label"] = "NxReleasePhysicsSDK"
        self.assertRejects(labels, "does not match the inventory label")

    def test_rejects_semantic_inventory_label_without_a_ledger_entry(self):
        labels = minimal_labels()
        labels["labels"] = []
        self.assertRejects(labels, "carries a semantic label without a ledger entry")

    def test_malformed_inventory_id_does_not_crash_the_ledger_check(self):
        inventory = minimal_inventory()
        inventory["functions"][1]["id"] = ["phys_fn_000002"]
        errors = validate_inventory.validate_labels(inventory, minimal_labels())
        self.assertRegex("\n".join(errors), "unknown stable ID 'phys_fn_000002'")

    def test_rejects_unknown_ledger_confidence(self):
        labels = minimal_labels()
        labels["labels"][0]["confidence"] = "guess"
        self.assertRejects(labels, "confidence 'guess' is not one of")

    def test_rejects_ledger_confidence_disagreeing_with_the_inventory(self):
        labels = minimal_labels()
        labels["labels"][0]["confidence"] = "stable-id"
        self.assertRejects(labels, "does not match the inventory label_confidence 'semantic'")

    def test_rejects_ledger_entry_without_evidence(self):
        labels = minimal_labels()
        labels["labels"][0]["evidence"] = ""
        self.assertRejects(labels, "must record evidence")

    def test_rejects_ledger_entry_without_reason(self):
        labels = minimal_labels()
        labels["labels"][0]["reason"] = ""
        self.assertRejects(labels, "must record reason")

    def test_rejects_unexpected_ledger_key(self):
        labels = minimal_labels()
        labels["author"] = "unknown"
        self.assertRejects(labels, "unexpected key 'author'")

    def test_labels_a_data_object(self):
        inventory = minimal_inventory()
        inventory["data_objects"][0]["label"] = "NxScene::vftable"
        inventory["data_objects"][0]["label_confidence"] = "semantic"
        labels = minimal_labels()
        labels["labels"].append(
            {
                "id": "phys_data_000001",
                "label": "NxScene::vftable",
                "confidence": "semantic",
                "evidence": "oracle/ghidra/manifest.json#vtables/0x00003000",
                "reason": "RTTI-backed vtable recovered by Ghidra",
            }
        )
        self.assertEqual(validate_inventory.validate_labels(inventory, labels), [])


class ValidateInventoryCliTests(unittest.TestCase):
    def run_cli(self, inventory, labels, program=None, maps=MINIMAL_MAPS, sources=PINNED_SOURCES):
        with tempfile.TemporaryDirectory() as directory:
            # The CLI reads program.json beside the inventory and the phase plans
            # from the repository root above it, so the layout has to be real.
            root = Path(directory) / "docs" / "reconstruction" / "novodex-physics"
            root.mkdir(parents=True)
            plans = Path(directory) / "docs" / "superpowers" / "plans"
            plans.mkdir(parents=True)
            for name in ("phase1.md", "phase2.md"):
                (plans / name).write_text("# plan\n", encoding="utf-8")
            if maps is not None:
                write_correspondence_maps(root, maps)
            if sources is not None:
                write_pinned_sources(Path(directory), sources)
            (root / "inventory.json").write_text(json.dumps(inventory), encoding="utf-8")
            (root / "labels.json").write_text(json.dumps(labels), encoding="utf-8")
            (root / "program.json").write_text(
                json.dumps(program if program is not None else minimal_program()),
                encoding="utf-8")
            return subprocess.run(
                [
                    sys.executable,
                    str(TOOLS_DIR / "validate_inventory.py"),
                    str(root / "inventory.json"),
                ],
                capture_output=True,
                text=True,
                check=False,
            )

    def test_valid_inventory_reports_counts_and_exits_zero(self):
        result = self.run_cli(minimal_inventory(), minimal_labels())
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("inventory=pass", result.stdout)
        self.assertIn("functions=2", result.stdout)
        self.assertIn("data_objects=1", result.stdout)
        self.assertIn("unexplained=0", result.stdout)

    def test_invalid_inventory_reports_errors_on_stderr_and_exits_one(self):
        inventory = minimal_inventory()
        inventory["functions"][0]["state"] = "in_progress"
        result = self.run_cli(inventory, minimal_labels())
        self.assertEqual(result.returncode, 1)
        self.assertIn("error:", result.stderr)
        self.assertIn("in_progress", result.stderr)
        self.assertNotIn("inventory=pass", result.stdout)

    def test_broken_label_ledger_fails_the_cli(self):
        labels = minimal_labels()
        labels["labels"][0]["id"] = "phys_fn_000404"
        result = self.run_cli(minimal_inventory(), labels)
        self.assertEqual(result.returncode, 1)
        self.assertIn("phys_fn_000404", result.stderr)

    def test_unreadable_input_reports_cli_error_without_traceback(self):
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "missing.json"
            result = subprocess.run(
                [sys.executable, str(TOOLS_DIR / "validate_inventory.py"), str(missing)],
                capture_output=True,
                text=True,
                check=False,
            )
        self.assertEqual(result.returncode, 2)
        self.assertIn("error:", result.stderr)
        self.assertNotIn("Traceback", result.stderr)

    def test_absent_correspondence_maps_fail_the_cli(self):
        # Without this the whole third-party gate passes vacuously the moment
        # the maps are moved or renamed, which is how a gate stops being able
        # to fail without anyone editing it.
        result = self.run_cli(minimal_inventory(), minimal_labels(), maps=None)
        self.assertEqual(result.returncode, 1)
        self.assertIn("cannot be read", result.stderr)
        self.assertNotIn("inventory=pass", result.stdout)

    def test_third_party_row_without_correspondence_fails_the_cli(self):
        inventory = minimal_inventory()
        inventory["functions"][0]["third_party"] = "qhull"
        result = self.run_cli(inventory, minimal_labels())
        self.assertEqual(result.returncode, 1)
        self.assertIn("no source correspondence map names it", result.stderr)


class ThirdPartyColumnTests(unittest.TestCase):
    """The third-party column and the correspondence maps must agree, both ways."""

    def correspondence(self, grade="mapped", library="qhull", rva="0x00001000"):
        return {"phys_fn_000001": (library, grade, rva, "qhull.c", "qh_qhull")}

    def declared(self, library="qhull", phase=validate_inventory.THIRD_PARTY_PHASE):
        inventory = minimal_inventory()
        inventory["phases"].append({"phase": phase, "name": "assets", "status": "pending"})
        inventory["functions"][0]["third_party"] = library
        inventory["functions"][0]["phase"] = phase
        return inventory

    def assertRejects(self, inventory, correspondence, pattern):
        errors = validate_inventory.validate_third_party(inventory, correspondence)
        self.assertTrue(errors, "expected at least one error")
        self.assertRegex("\n".join(errors), pattern)

    def test_a_declared_row_with_a_mapped_correspondence_is_accepted(self):
        self.assertEqual(
            validate_inventory.validate_third_party(self.declared(), self.correspondence()), []
        )

    def test_a_row_with_no_correspondence_may_not_be_declared(self):
        self.assertRejects(self.declared(), {}, "no source correspondence map names it")

    def test_an_unmapped_correspondence_may_not_be_declared(self):
        # The 185 rows of NovodeX code inside the two library spans grade
        # `unmapped`, and relabelling them is the cheapest way to make the
        # third-party total come out round.
        self.assertRejects(self.declared(), self.correspondence(grade="unmapped"),
                           "measured absence of an upstream counterpart")

    def test_a_correspondence_left_undeclared_is_rejected(self):
        inventory = minimal_inventory()
        self.assertRejects(inventory, self.correspondence(),
                           r"corresponds to qhull's qhull\.c but the census declares "
                           r"third_party=None")

    def test_a_probable_correspondence_is_enough_to_declare(self):
        self.assertEqual(
            validate_inventory.validate_third_party(
                self.declared(), self.correspondence(grade="probable")), []
        )

    def test_a_declared_row_outside_the_third_party_phase_is_rejected(self):
        self.assertRejects(self.declared(phase=2), self.correspondence(),
                           "will be asked to reconstruct it")

    def test_a_declaration_naming_an_unknown_library_is_rejected(self):
        self.assertRejects(self.declared(library="ode"), self.correspondence(library="ode"),
                           "declares third_party='ode', which is not one of")

    def test_a_correspondence_for_a_row_that_is_not_censused_is_rejected(self):
        correspondence = {"phys_fn_000404": ("qhull", "mapped", "0x00009000", "io.c", "qh_printvdiagram")}
        self.assertRejects(minimal_inventory(), correspondence,
                           "which is not a censused function row")

    def test_a_correspondence_whose_rva_disagrees_with_the_census_is_rejected(self):
        self.assertRejects(self.declared(), self.correspondence(rva="0x00009000"),
                           "puts phys_fn_000001 at 0x00009000 but the census has it at 0x00001000")


class SourceCorrespondenceMapTests(unittest.TestCase):
    """The maps are read off disk, so a malformed one has to be a stated defect."""

    def read(self, maps, sources=PINNED_SOURCES):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_correspondence_maps(root, maps)
            if sources is not None:
                write_pinned_sources(root, sources)
            return validate_inventory.read_source_correspondence(root)

    def test_reads_every_library(self):
        entries, errors = self.read({
            "qhull_map.csv": [map_row("phys_fn_000001", "0x00001000")],
            "opcode_map.csv": [map_row("phys_fn_000002", "0x00001010",
                                       source_file="OPC_Model.cpp",
                                       source_function="Model::~Model")],
            "opcode_outside_span_map.csv": [map_row("phys_fn_000003", "0x00001030",
                                                    source_file="Ice/IceContainer.cpp",
                                                    source_function="Container::Container()")],
        })
        self.assertEqual(errors, [])
        self.assertEqual({key: value[0] for key, value in entries.items()},
                         {"phys_fn_000001": "qhull", "phys_fn_000002": "opcode",
                          "phys_fn_000003": "opcode"})

    def test_a_missing_map_is_an_error_not_an_empty_result(self):
        entries, errors = self.read({"qhull_map.csv": []})
        self.assertEqual(entries, {})
        self.assertRegex("\n".join(errors), "opcode_map.csv cannot be read")

    def test_a_map_missing_a_column_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_correspondence_maps(root, MINIMAL_MAPS)
            write_pinned_sources(root)
            (root / validate_inventory.THIRD_PARTY_MAP_DIR / "qhull_map.csv").write_text(
                "rva,id\n0x00001000,phys_fn_000001\n", encoding="utf-8")
            _, errors = validate_inventory.read_source_correspondence(root)
        self.assertRegex("\n".join(errors), "missing column\\(s\\) grade")

    def test_an_unknown_grade_is_rejected(self):
        _, errors = self.read(dict(MINIMAL_MAPS, **{
            "qhull_map.csv": [map_row("phys_fn_000001", "0x00001000", grade="probably")]}))
        self.assertRegex("\n".join(errors), "grades phys_fn_000001 'probably'")

    def test_a_row_graded_by_two_libraries_is_rejected(self):
        _, errors = self.read(dict(MINIMAL_MAPS, **{
            "qhull_map.csv": [map_row("phys_fn_000001", "0x00001000")],
            "opcode_map.csv": [map_row("phys_fn_000001", "0x00001000",
                                       source_file="OPC_Model.cpp")],
        }))
        self.assertRegex("\n".join(errors), "is graded twice")

    def test_a_map_entry_that_is_not_a_stable_id_is_rejected(self):
        _, errors = self.read(dict(MINIMAL_MAPS, **{
            "qhull_map.csv": [map_row("qh_qhull", "0x00001000")]}))
        self.assertRegex("\n".join(errors), "which is not a phys_fn_%06d stable ID")


class PinnedSourceBindingTests(unittest.TestCase):
    """A correspondence must name a function the pinned upstream tree supplies.

    This is the half of the map that can be bound mechanically. The other half
    -- that the named function is what those bytes actually are -- still rests
    on a human re-derivation from the disassembly, and no check here changes
    that. What it removes is the free-text claim: a row that corresponds to
    nothing a vendored tree contains can no longer be graded and declared.
    """

    def read(self, rows, sources=PINNED_SOURCES, library="qhull_map.csv"):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_correspondence_maps(root, dict(MINIMAL_MAPS, **{library: rows}))
            if sources is not None:
                write_pinned_sources(root, sources)
            return validate_inventory.read_source_correspondence(root)[1]

    def assertRejects(self, rows, pattern, **kwargs):
        errors = self.read(rows, **kwargs)
        self.assertTrue(errors, "expected at least one error")
        self.assertRegex("\n".join(errors), pattern)

    def test_a_correspondence_the_pinned_tree_supplies_is_accepted(self):
        self.assertEqual(self.read([map_row("phys_fn_000001", "0x00001000")]), [])

    def test_absent_pinned_trees_are_an_error_not_an_empty_result(self):
        # Same failure mode as an absent map: the check would pass because its
        # input is gone, which is how a gate stops being able to fail.
        self.assertRejects([map_row("phys_fn_000001", "0x00001000")],
                           "pinned upstream source trees are not staged", sources=None)

    def test_a_source_file_outside_the_pinned_tree_is_rejected(self):
        self.assertRejects([map_row("phys_fn_000001", "0x00001000", source_file="qh_made_up.c")],
                           "which is not a file in the pinned qhull tree")

    def test_a_source_function_the_pinned_file_lacks_is_rejected(self):
        # phys_fn_005177 in the real map was exactly this: a RadixSort member
        # NovodeX added, graded against a header that does not declare it.
        self.assertRejects(
            [map_row("phys_fn_000001", "0x00001000", source_function="qh_never_existed")],
            "does not contain 'qh_never_existed'")

    def test_a_source_function_that_is_free_text_is_rejected(self):
        self.assertRejects(
            [map_row("phys_fn_000001", "0x00001000", source_function="qhull setup helper")],
            "which does not name an upstream source function")

    def test_a_blank_source_file_is_rejected(self):
        self.assertRejects([map_row("phys_fn_000001", "0x00001000", source_file="")],
                           "with no source_file")

    def test_a_blank_source_function_is_rejected(self):
        self.assertRejects([map_row("phys_fn_000001", "0x00001000", source_function="")],
                           "with no source_function")

    def test_a_blank_rva_leaves_the_row_unchecked_and_is_rejected(self):
        self.assertRejects([map_row("phys_fn_000001", "")],
                           "grades phys_fn_000001 with no rva")

    def test_a_source_file_escaping_the_pinned_tree_is_rejected(self):
        self.assertRejects(
            [map_row("phys_fn_000001", "0x00001000", source_file="../../qhull.c")],
            "which is not a path inside the pinned qhull tree")

    def test_regrading_an_unmapped_row_in_place_is_rejected(self):
        # The cheapest attack on the 186 unmapped rows: flip one cell from
        # `unmapped` to `mapped` and let the census declare the library. Their
        # source_file names a file no pinned tree has, so the flip is caught.
        rows = [map_row("phys_fn_000001", "0x00001000", grade="unmapped",
                        source_file="IcePrunable.cpp (NovodeX; absent from OPCODE 1.3)",
                        source_function="Prunable::Prunable")]
        self.assertEqual(self.read(rows), [])
        rows[0]["grade"] = "mapped"
        self.assertRejects(rows, "which is not a file in the pinned qhull tree")

    def test_an_unmapped_row_is_not_bound_to_the_pinned_tree(self):
        # `unmapped` is the measured absence of an upstream counterpart, so its
        # source_file is a description rather than a path, and must stay free.
        self.assertEqual(
            self.read([map_row("phys_fn_000001", "0x00001000", grade="unmapped",
                               source_file="IcePrunable.cpp (NovodeX)",
                               source_function="a NovodeX addition")]), [])

    def test_a_deleting_destructor_thunk_resolves_against_its_class(self):
        # MSVC generates the thunk; what a vendored tree supplies is the class
        # and its destructor. 13 rows of the real map are this shape.
        self.assertEqual(
            self.read([map_row("phys_fn_000002", "0x00001010", source_file="OPC_Model.cpp",
                               source_function="Model::`scalar deleting destructor'")],
                      library="opcode_map.csv"), [])


class SourceIdentifierTests(unittest.TestCase):
    """What the map's source_function column has to reduce to, spelling by spelling."""

    def check(self, source_function, expected):
        self.assertEqual(validate_inventory.source_identifier(source_function), expected)

    def test_a_plain_c_function(self):
        self.check("qh_initflags", "qh_initflags")

    def test_a_qualified_member_with_a_parameter_list(self):
        self.check("RadixSort::Sort(const udword*, udword, RadixHint)", "Sort")

    def test_a_return_type_is_not_mistaken_for_the_name(self):
        self.check("Container& Container::Empty()", "Empty")

    def test_an_annotation_is_dropped(self):
        self.check("AABBCollisionTree::GetUsedBytes [IMPLEMENT_COLLISION_TREE]", "GetUsedBytes")

    def test_a_destructor_reduces_to_its_class(self):
        self.check("Container::~Container()", "Container")

    def test_a_deleting_destructor_thunk_reduces_to_its_class(self):
        self.check("RayCollider::`scalar deleting destructor'", "RayCollider")
        self.check("Model::~Model / `scalar deleting destructor'", "Model")

    def test_free_text_reduces_to_nothing(self):
        self.check("SweepAndPrune batch-update helper", "")
        self.check("LSSCollider segment/LSS setup helper", "")


class ClosureLedgerTests(unittest.TestCase):
    """A row closes only if a mutation was aimed at it and the gate caught it."""

    # `by_phase` is the other half of the registry: which phase's gate run
    # actually executes each target. Phase 2 owns the fixture's own gates and
    # phase 3 owns NxPhysicsCollisionTests, so a phase 2 row naming the latter
    # is a row resting on a gate phase 2 never runs.
    TARGETS = {"differential": {"NxRealDifferential", "NxPhysicsGeometryTests",
                                "NxPhysicsKernelFuzzTests"},
               "static_proof": {"NxRealStaticProof"},
               "oracle_differential": {"NxPhysicsCollisionTests"},
               "by_phase": {"differential": {2: {"NxRealDifferential",
                                                 "NxPhysicsGeometryTests",
                                                 "NxPhysicsKernelFuzzTests"}},
                            "static_proof": {2: {"NxRealStaticProof"}},
                            "oracle_differential": {3: {"NxPhysicsCollisionTests"}}}}

    def ledger(self, **overrides):
        ledger = {
            "schema_version": 1, "phase": 2, "note": "", "proof_kinds": {},
            "deferred_reasons": {}, "measured_from": {},
            "counts": {"differential_falsified": 1, "static_proof_falsified": 1,
                       "deferred_blocked_on_later_phase": 1,
                       "deferred_data_object_not_dispositioned": 1},
            "closed": [
                {"id": "phys_fn_000001", "rva": "0x00001000", "proof": "differential_falsified",
                 "gate": "NxRealDifferential",
                 "falsification": {"mutation": "returns 0", "detected": "stdout_delta=4"}},
                {"id": "phys_fn_000002", "rva": "0x00001030", "proof": "static_proof_falsified",
                 "gate": "NxRealStaticProof",
                 "falsification": {"mutation": "drops the guard",
                                   "detected": "check_failed the guard holds"}}],
            "deferred": [
                {"id": "phys_fn_000003", "rva": "0x00001060", "size": 8,
                 "phase_provenance": "translation_unit",
                 "reason": "blocked_on_later_phase", "driving_phases": [4],
                 "blocked_on": ["phys_fn_000004"], "blocked_on_type": "Scene"},
                {"id": "phys_data_000001", "rva": "0x00002000", "size": 4,
                 "phase_provenance": "reading_sites",
                 "reason": "data_object_not_dispositioned", "driving_phases": []}],
        }
        ledger.update(overrides)
        return ledger

    def inventory(self, **states):
        """The state on each row is the rung its ledger entry supports: a differential closure is
        dynamically gated, a static proof is not, and a deferred row has no gate that caught it.

        `states` overrides one row's state by stable ID, so a test can place a row on a terminal rung
        without `errors()` building a fresh fixture and losing the change.
        """
        data = {"functions": [{"id": "phys_fn_000001", "phase": 2, "rva": "0x00001000",
                               "state": "dynamically_gated"},
                              {"id": "phys_fn_000002", "phase": 2, "rva": "0x00001030",
                               "state": "statically_reviewed"},
                              {"id": "phys_fn_000003", "phase": 2, "rva": "0x00001060",
                               "state": "discovered"},
                              {"id": "phys_fn_000004", "phase": 4, "rva": "0x00001090",
                               "state": "discovered"}],
                "data_objects": [{"id": "phys_data_000001", "phase": 2, "rva": "0x00002000",
                                  "state": "discovered"},
                                 {"id": "phys_data_000002", "phase": 3, "rva": "0x00002010",
                                  "state": "discovered"}]}
        for row in data["functions"] + data["data_objects"]:
            if row["id"] in states:
                row["state"] = states[row["id"]]
        return data

    def errors(self, ledger, phase=2, **states):
        return validate_inventory.validate_closure(
            self.inventory(**states), ledger, phase, self.TARGETS)

    def assertRejects(self, ledger, message, phase=2, **states):
        errors = self.errors(ledger, phase, **states)
        self.assertTrue(any(re.search(message, error) for error in errors),
                        f"{message!r} not in {errors}")

    def test_accepts_a_ledger_that_accounts_for_every_row_of_the_phase(self):
        self.assertEqual(self.errors(self.ledger()), [])

    def test_there_is_no_proof_kind_that_skips_the_mutation(self):
        # The kind that rested on a free-text observation is gone; every error
        # found in review was one of those rows.
        self.assertEqual(validate_inventory.PROOF_KINDS,
                         ("differential_falsified", "static_proof_falsified",
                          "oracle_differential_falsified"))
        # Every kind ends in `_falsified`, which is the property this test is
        # for: adding a kind must not add a way to close a row without a
        # mutation that was measured to be caught.
        for kind in validate_inventory.PROOF_KINDS:
            self.assertTrue(kind.endswith("_falsified"), kind)
        ledger = self.ledger()
        ledger["closed"][0]["proof"] = "differential_observed"
        self.assertRejects(ledger, "claims proof 'differential_observed'")

    def test_an_oracle_differential_closure_needs_a_non_zero_mismatch_count(self):
        # It runs once, so it has no second transcript and cannot borrow
        # stdout_delta from a runner that never touched the row.
        ledger = self.ledger()
        row = ledger["closed"][0]
        row["proof"] = "oracle_differential_falsified"
        row["gate"] = "NxPhysicsCollisionTests"
        ledger["counts"] = {"oracle_differential_falsified": 1, "static_proof_falsified": 1,
                            "deferred_blocked_on_later_phase": 1}
        row["falsification"]["detected"] = "stdout_delta=4"
        self.assertRejects(ledger, "has no transcript to diff")
        row["falsification"]["detected"] = "mismatches=0"
        self.assertRejects(ledger, "not a non-zero mismatches count")
        row["falsification"]["detected"] = "check_failed something"
        self.assertRejects(ledger, "not a non-zero mismatches count")

    def test_a_differential_closure_may_report_the_unit_its_probe_measured(self):
        # NxPhysicsGeometryTests prints one line per case and
        # NxPhysicsKernelFuzzTests one digest per export, and a per-row mutation
        # probe is compared against the committed oracle transcript rather than
        # run through run_differential.ps1, so what it has is a count of
        # differing cases or digests. Converting that into a whole-target
        # stdout_delta would be arithmetic on Compare-Object's semantics
        # presented as a measurement.
        for unit, gate in (("cases", "NxPhysicsGeometryTests"),
                           ("digests", "NxPhysicsKernelFuzzTests")):
            ledger = self.ledger()
            ledger["closed"][0]["gate"] = gate
            ledger["closed"][0]["falsification"]["detected"] = f"{unit}=2"
            self.assertEqual(self.errors(ledger), [])

    def test_a_differential_unit_has_to_be_one_the_gate_it_names_can_print(self):
        # The unit is recorded so a reader knows which comparison produced it,
        # which is worth nothing if the named gate never performs that
        # comparison. NxPhysicsGeometryTests prints one line per case and
        # NxPhysicsKernelFuzzTests one digest per export, so the two units do
        # not travel.
        for unit, wrong_gate in (("cases", "NxPhysicsKernelFuzzTests"),
                                 ("digests", "NxPhysicsGeometryTests"),
                                 ("cases", "NxRealDifferential"),
                                 ("digests", "NxRealDifferential")):
            ledger = self.ledger()
            ledger["closed"][0]["gate"] = wrong_gate
            ledger["closed"][0]["falsification"]["detected"] = f"{unit}=2"
            self.assertRejects(ledger, f"does not print {unit}")

    def test_the_whole_target_delta_is_not_bound_to_one_gate(self):
        # `stdout_delta` is run_differential.ps1's number for a whole target, so
        # every staged-pair gate has one and binding it to a single harness
        # would reject rows that are correct.
        for gate in ("NxRealDifferential", "NxPhysicsGeometryTests",
                     "NxPhysicsKernelFuzzTests"):
            ledger = self.ledger()
            ledger["closed"][0]["gate"] = gate
            ledger["closed"][0]["falsification"]["detected"] = "stdout_delta=4"
            self.assertEqual(self.errors(ledger), [])

    def test_a_differential_unit_still_has_to_be_a_canonical_non_zero_count(self):
        # The unit is new; the requirement that something was caught is not.
        for detected in ("cases=0", "digests=0", "cases=007", "digests=", "cases", "hits=3"):
            ledger = self.ledger()
            ledger["closed"][0]["falsification"]["detected"] = detected
            self.assertRejects(ledger, "not a non-zero stdout_delta, cases or digests count")

    def test_a_staged_pair_differential_may_not_report_a_mismatch_count(self):
        # `mismatches=` belongs to the in-process oracle differential. A staged
        # pair runs one binary against two pairs and has no such number.
        ledger = self.ledger()
        ledger["closed"][0]["falsification"]["detected"] = "mismatches=42"
        self.assertRejects(ledger, "not a non-zero stdout_delta, cases or digests count")

    def test_a_gate_registered_to_another_phase_cannot_close_this_phase_s_row(self):
        # Gate names used to be kind-scoped and not phase-scoped, so a Phase 4
        # row could close on NxPhysicsCollisionTests -- Phase 3's oracle
        # differential, which does not run at all when Phase 4 is gated -- and
        # every other check passed. Found by the Phase 4 falsification pass, and
        # when it was ported it caught one real closure: phys_fn_002344, a
        # Phase 3 row closed on Phase 5's NxPhysicsObjectLayoutTests.
        ledger = self.ledger()
        ledger["closed"][0]["proof"] = "oracle_differential_falsified"
        ledger["closed"][0]["gate"] = "NxPhysicsCollisionTests"
        ledger["closed"][0]["falsification"]["detected"] = "mismatches=4"
        ledger["counts"] = {"oracle_differential_falsified": 1, "static_proof_falsified": 1,
                            "deferred_blocked_on_later_phase": 1,
                            "deferred_data_object_not_dispositioned": 1}
        self.assertRejects(ledger, "registered to phase 3 and not to phase 2")

    def test_a_discharged_row_may_name_the_discharging_phase_s_gate(self):
        # The one legitimate cross-phase case, and it is not hypothetical:
        # gates/phase2-closure.json closes phys_fn_001690 on Phase 3's
        # NxPhysicsCollisionTests with discharged_by_phase 3.
        ledger = self.ledger()
        ledger["closed"][0]["proof"] = "oracle_differential_falsified"
        ledger["closed"][0]["gate"] = "NxPhysicsCollisionTests"
        ledger["closed"][0]["falsification"]["detected"] = "mismatches=4"
        ledger["closed"][0]["discharged_by_phase"] = 3
        ledger["closed"][0]["driving_phases"] = [3]
        ledger["counts"] = {"oracle_differential_falsified": 1, "static_proof_falsified": 1,
                            "deferred_blocked_on_later_phase": 1,
                            "deferred_data_object_not_dispositioned": 1}
        self.assertEqual(self.errors(ledger), [])

    def test_a_phase_may_defer_a_row_nobody_got_to_without_claiming_it_is_blocked(self):
        # Phase 2 reached 125 of its 163 rows, so every row it did not close was
        # blocked, homeless or measured unreachable. Phase 3 owns 441 and closed
        # 61: most of the rest is a phase that ran out of dispatches, which is a
        # debt and not an obstruction, and naming a later phase for it would
        # invent an obligation nobody owes.
        # Phase 4 owns 1,137 and 722 of its rows were vendored rather than
        # reconstructed at all: `vendored_not_falsified` says the upstream source
        # is in the build and no mutation is aimed at the row, and
        # `vendored_driven_divergent` says it was driven and did not reproduce
        # the image. Neither may name a later phase either.
        for reason in ("not_reconstructed_in_phase", "reconstructed_not_falsified",
                       "vendored_not_falsified", "vendored_driven_divergent"):
            ledger = self.ledger()
            ledger["deferred"][0] = {"id": "phys_fn_000003", "rva": "0x00001060", "size": 8,
                                     "phase_provenance": "translation_unit",
                                     "reason": reason, "driving_phases": []}
            ledger["counts"] = {"differential_falsified": 1, "static_proof_falsified": 1,
                                "deferred_" + reason: 1,
                                "deferred_data_object_not_dispositioned": 1}
            # A vendored reason is a claim about a third-party row, so the row
            # it is given for has to be one.
            vendored = reason in validate_inventory.VENDORED_DEFERRAL_REASONS
            self.assertEqual(self.vendored_errors(ledger, vendored=vendored), [])

    # -- the vendored split ---------------------------------------------------
    #
    # `vendored_not_falsified` is for a vendored row the census leaves at
    # `discovered`, and `not_reconstructed_in_phase` is false about such a row.
    # Both relabellings validated with the counts adjusted while the rule lived
    # only in the ledger's prose.

    def vendored_errors(self, ledger, vendored=True, state="discovered"):
        inventory = self.inventory(phys_fn_000003=state)
        if vendored:
            inventory["functions"][2]["third_party"] = "qhull"
        return validate_inventory.validate_closure(inventory, ledger, 2, self.TARGETS)

    def deferred_as(self, reason):
        ledger = self.ledger()
        ledger["deferred"][0] = {"id": "phys_fn_000003", "rva": "0x00001060", "size": 8,
                                 "phase_provenance": "translation_unit",
                                 "reason": reason, "driving_phases": []}
        ledger["counts"] = {"differential_falsified": 1, "static_proof_falsified": 1,
                            "deferred_" + reason: 1,
                            "deferred_data_object_not_dispositioned": 1}
        return ledger

    def test_rejects_a_novodex_row_relabelled_vendored(self):
        errors = self.vendored_errors(self.deferred_as("vendored_not_falsified"), vendored=False)
        self.assertIn("declares no third_party", "\n".join(errors))

    def test_rejects_a_novodex_row_relabelled_vendored_and_divergent(self):
        errors = self.vendored_errors(self.deferred_as("vendored_driven_divergent"),
                                      vendored=False)
        self.assertIn("declares no third_party", "\n".join(errors))

    def test_rejects_vendored_not_falsified_on_a_row_the_census_raised(self):
        # A vendored row a later drive raised to `reconstructed` keeps
        # `reconstructed_not_falsified`; the vendored reason is for `discovered`.
        errors = self.vendored_errors(self.deferred_as("vendored_not_falsified"),
                                      state="reconstructed")
        self.assertIn("the reason is for a vendored row the census leaves at 'discovered'",
                      "\n".join(errors))

    def test_rejects_a_vendored_row_at_discovered_relabelled_not_reconstructed(self):
        errors = self.vendored_errors(self.deferred_as("not_reconstructed_in_phase"))
        self.assertIn("is a third-party row at 'discovered' giving not_reconstructed_in_phase",
                      "\n".join(errors))

    def test_a_raised_vendored_row_keeps_reconstructed_not_falsified(self):
        self.assertEqual(self.vendored_errors(self.deferred_as("reconstructed_not_falsified"),
                                              state="reconstructed"), [])

    def test_the_new_reasons_stay_on_the_code_half_of_the_census(self):
        # A data object giving a code reason claims a reachability argument
        # nobody made about it, and it would shrink the data debt silently. The
        # two vendored reasons are the ones that would do it most quietly: a data
        # object relabelled `vendored_not_falsified` would read as part of a
        # third-party library rather than as an undispositioned object.
        for reason in ("not_reconstructed_in_phase", "reconstructed_not_falsified",
                       "vendored_not_falsified", "vendored_driven_divergent"):
            ledger = self.ledger()
            ledger["deferred"][1]["reason"] = reason
            ledger["counts"] = {"differential_falsified": 1, "static_proof_falsified": 1,
                                "deferred_blocked_on_later_phase": 1,
                                "deferred_" + reason: 1}
            self.assertRejects(ledger, f"is a data object giving {reason}")

    def test_rejects_a_closure_that_records_no_mutation(self):
        ledger = self.ledger()
        del ledger["closed"][0]["falsification"]
        self.assertRejects(ledger, "records no mutation and detection")

    def test_rejects_a_falsification_that_detected_nothing(self):
        ledger = self.ledger()
        ledger["closed"][0]["falsification"]["detected"] = "stdout_delta=0"
        self.assertRejects(ledger, "not a non-zero stdout_delta")

    def test_rejects_a_static_proof_claiming_a_transcript_delta(self):
        # A static proof has no oracle side, so it has no delta to report.
        ledger = self.ledger()
        ledger["closed"][1]["falsification"]["detected"] = "stdout_delta=5"
        self.assertRejects(ledger, "no transcript to take a delta from")

    def test_rejects_a_static_proof_detection_that_names_no_check(self):
        ledger = self.ledger()
        ledger["closed"][1]["falsification"]["detected"] = "x"
        self.assertRejects(ledger, "does not name the check that failed")

    def test_rejects_a_data_object_claiming_a_code_proof(self):
        # A data object has no body to mutate; letting it claim one inflated the
        # headline counter with rows nobody could have falsified.
        ledger = self.ledger()
        ledger["closed"].append({"id": "phys_data_000001", "rva": "0x00002000",
                                 "proof": "differential_falsified", "gate": "NxRealDifferential",
                                 "falsification": {"mutation": "m", "detected": "stdout_delta=2"}})
        ledger["deferred"] = [ledger["deferred"][0]]
        ledger["counts"] = {"differential_falsified": 2, "static_proof_falsified": 1,
                            "deferred_blocked_on_later_phase": 1}
        self.assertRejects(ledger, "is a data object claiming differential_falsified")

    def test_rejects_a_phase_row_that_is_neither_closed_nor_deferred(self):
        ledger = self.ledger()
        ledger["deferred"] = [ledger["deferred"][1]]
        del ledger["counts"]["deferred_blocked_on_later_phase"]
        self.assertRejects(ledger, "neither closed, deferred, nor terminal: phys_fn_000003")

    def test_a_data_object_is_inside_the_ledgers_jurisdiction(self):
        ledger = self.ledger()
        ledger["deferred"] = [ledger["deferred"][0]]
        del ledger["counts"]["deferred_data_object_not_dispositioned"]
        # The row stays off its terminal rung, because a terminal row is now accounted for
        # without appearing in either list and would prove nothing about the partition.
        self.assertRejects(ledger, "neither closed, deferred, nor terminal: phys_data_000001")

    def test_rejects_a_deferral_on_a_row_whose_evidence_is_complete(self):
        """The contradiction 15j measured: the ledger says unfinished, the census says finished.

        The fixture already defers the data object, so this changes nothing but the row's state --
        which is the whole of the contradiction.
        """
        ledger = self.ledger()
        self.assertRejects(ledger, "not unfinished, so the deferral and the state disagree",
                           **{"phys_data_000001": "classified"})

    def test_rejects_a_row_that_is_both_closed_and_deferred(self):
        ledger = self.ledger()
        ledger["deferred"].append({"id": "phys_fn_000001", "rva": "0x00001000", "size": 4,
                                   "phase_provenance": "shared_by_callers",
                                   "reason": "homeless_shared_code", "driving_phases": []})
        ledger["counts"]["deferred_homeless_shared_code"] = 1
        self.assertRejects(ledger, "both closed and deferred: phys_fn_000001")

    def test_rejects_a_stale_key_left_on_a_closed_entry(self):
        # The deleted "observed" kind left `observed_in` fields behind that
        # still parsed, so a closure could carry its evidence somewhere the
        # validator does not read.
        ledger = self.ledger()
        ledger["closed"][0]["observed_in"] = "step=create"
        self.assertRejects(ledger, "has unexpected key 'observed_in'")

    def test_a_closed_entry_may_carry_a_note(self):
        # A deferred entry has always had room to say something about itself
        # that is not its reason; a closed one had none, so the one closure in
        # the set that is thin enough to want flagging -- seven bytes, six of
        # them an uncomparable vptr store, a delta of 1 -- could only be flagged
        # in a document nobody diffs against the ledger.
        ledger = self.ledger()
        ledger["closed"][0]["note"] = "marginal: six of its seven bytes are a vptr store"
        self.assertEqual(self.errors(ledger), [])

    def test_rejects_a_stale_key_left_on_a_deferred_entry(self):
        ledger = self.ledger()
        ledger["deferred"][0]["observed_in"] = "step=create"
        self.assertRejects(ledger, "has unexpected key 'observed_in'")

    def test_rejects_a_stale_key_left_inside_a_falsification(self):
        ledger = self.ledger()
        ledger["closed"][0]["falsification"]["observed_in"] = "step=create"
        self.assertRejects(ledger, "has unexpected key 'observed_in'")

    def test_rejects_a_mutation_that_is_only_whitespace(self):
        # `" "` satisfied the old truth test, so a row could be closed on a
        # string nobody had to write.
        ledger = self.ledger()
        ledger["closed"][0]["falsification"]["mutation"] = "   "
        self.assertRejects(ledger, "records no mutation and detection")

    def test_rejects_a_mutation_that_is_not_a_string(self):
        ledger = self.ledger()
        ledger["closed"][0]["falsification"]["mutation"] = 1
        self.assertRejects(ledger, "records no mutation and detection")

    def test_rejects_a_delta_written_in_a_form_no_runner_emits(self):
        # `stdout_delta=007` parsed as 7 and passed; no runner prints that.
        ledger = self.ledger()
        ledger["closed"][0]["falsification"]["detected"] = "stdout_delta=007"
        self.assertRejects(ledger, "not a non-zero stdout_delta")

    def test_a_list_shaped_falsification_is_reported_not_raised(self):
        # It failed closed, but on a traceback rather than on a stated defect.
        ledger = self.ledger()
        ledger["closed"][0]["falsification"] = ["returns 0"]
        self.assertRejects(ledger, "which is not a mutation and a detection")

    def test_a_closed_entry_with_no_id_is_reported_not_raised(self):
        ledger = self.ledger()
        del ledger["closed"][0]["id"]
        self.assertRejects(ledger, "does not name a stable ID")

    def test_a_deferred_entry_with_no_id_is_reported_not_raised(self):
        ledger = self.ledger()
        del ledger["deferred"][0]["id"]
        self.assertRejects(ledger, "does not name a stable ID")

    def test_rejects_a_closed_entry_quoting_another_rows_address(self):
        # Never compared, so a closure could name one row and quote another's
        # RVA; the reader checking the disassembly would find the wrong bytes
        # consistent.
        ledger = self.ledger()
        ledger["closed"][0]["rva"] = "0x00001090"
        self.assertRejects(ledger, "records rva '0x00001090' but the inventory has '0x00001000'")

    def test_rejects_a_deferred_entry_quoting_another_rows_address(self):
        ledger = self.ledger()
        ledger["deferred"][0]["rva"] = "0x00001000"
        self.assertRejects(ledger, "records rva '0x00001000' but the inventory has '0x00001060'")

    def test_rejects_a_function_row_deferred_into_the_data_debt(self):
        # The deferral reasons were policed in neither direction, so the split
        # between the code half and the data half was fabricable.
        ledger = self.ledger()
        ledger["deferred"][0]["reason"] = "data_object_not_dispositioned"
        ledger["counts"] = {"differential_falsified": 1, "static_proof_falsified": 1,
                            "deferred_data_object_not_dispositioned": 2}
        self.assertRejects(ledger, "is a function row giving data_object_not_dispositioned")

    def test_rejects_a_data_object_giving_a_code_reachability_reason(self):
        ledger = self.ledger()
        ledger["deferred"][1]["reason"] = "homeless_shared_code"
        ledger["counts"] = {"differential_falsified": 1, "static_proof_falsified": 1,
                            "deferred_blocked_on_later_phase": 1,
                            "deferred_homeless_shared_code": 1}
        self.assertRejects(ledger, "is a data object giving homeless_shared_code")

    def test_rejects_a_static_proof_row_the_inventory_calls_dynamically_gated(self):
        # The state vocabulary is what carries the distinction between a proof
        # that ran against the oracle and one derived from a disassembly.
        inventory = self.inventory()
        inventory["functions"][1]["state"] = "dynamically_gated"
        errors = validate_inventory.validate_closure(
            inventory, self.ledger(), 2, self.TARGETS)
        self.assertTrue(any("closes on a static proof but the inventory leaves it "
                            "'dynamically_gated'" in error for error in errors), errors)

    def test_rejects_a_differential_closure_the_inventory_leaves_discovered(self):
        inventory = self.inventory()
        inventory["functions"][0]["state"] = "discovered"
        errors = validate_inventory.validate_closure(
            inventory, self.ledger(), 2, self.TARGETS)
        self.assertTrue(any("carries a differential falsification but the inventory leaves it "
                            "'discovered'" in error for error in errors), errors)

    def test_rejects_a_static_closure_the_inventory_leaves_discovered(self):
        inventory = self.inventory()
        inventory["functions"][1]["state"] = "discovered"
        errors = validate_inventory.validate_closure(
            inventory, self.ledger(), 2, self.TARGETS)
        self.assertTrue(any("carries a static proof but the inventory leaves it 'discovered'"
                            in error for error in errors), errors)

    def test_rejects_a_deferred_row_the_inventory_says_a_gate_caught(self):
        inventory = self.inventory()
        inventory["functions"][2]["state"] = "closed"
        errors = validate_inventory.validate_closure(
            inventory, self.ledger(), 2, self.TARGETS)
        self.assertTrue(any("is deferred but the inventory leaves it 'closed'" in error
                            for error in errors), errors)

    def test_rejects_a_gate_that_is_not_a_registered_target(self):
        ledger = self.ledger()
        ledger["closed"][0]["gate"] = "TotallyRealGate"
        self.assertRejects(ledger, "not a registered differential target")

    def test_rejects_a_static_proof_claimed_against_a_differential_target(self):
        ledger = self.ledger()
        ledger["closed"][1]["gate"] = "NxRealDifferential"
        self.assertRejects(ledger, "not a registered static_proof target")

    def test_rejects_counts_that_do_not_match_the_entries(self):
        ledger = self.ledger()
        ledger["counts"]["differential_falsified"] = 999
        self.assertRejects(ledger, "counts do not match its own entries")

    # -- a deferral discharged by a later phase ----------------------------
    #
    # Phase 2 deferred 1,155 rows, each naming the phases that could discharge
    # it, and until phys_fn_001690 nothing recorded that one ever had. The two
    # rules that made the census trustworthy are also what made a discharge
    # unrecordable: `stray` stops phase N's ledger naming another phase's row and
    # `unaccounted` stops the owning phase's ledger dropping it. So the discharge
    # goes on the *owning* phase's ledger, moving the row from deferred to
    # closed, with the phase that produced the proof named and the original
    # driving_phases travelling with it so the claim can be checked.

    def discharged(self):
        ledger = self.ledger()
        row = ledger["closed"][0]
        row["proof"] = "oracle_differential_falsified"
        row["gate"] = "NxPhysicsCollisionTests"
        row["falsification"]["detected"] = "mismatches=4"
        row["discharged_by_phase"] = 3
        row["driving_phases"] = [3, 4]
        ledger["counts"] = {"oracle_differential_falsified": 1, "static_proof_falsified": 1,
                            "deferred_blocked_on_later_phase": 1,
                            "deferred_data_object_not_dispositioned": 1}
        return ledger

    def test_accepts_a_deferral_discharged_by_a_phase_the_deferral_named(self):
        self.assertEqual(self.errors(self.discharged()), [])

    def test_rejects_a_discharge_by_a_phase_the_deferral_never_named(self):
        # Without this a phase could adopt any row it liked by writing its own
        # number in, which is exactly what `stray` exists to prevent.
        ledger = self.discharged()
        ledger["closed"][0]["discharged_by_phase"] = 5
        self.assertRejects(ledger, "the deferral named 3, 4 as the phases that could")

    def test_rejects_a_phase_discharging_its_own_deferral(self):
        ledger = self.discharged()
        ledger["closed"][0]["discharged_by_phase"] = 2
        ledger["closed"][0]["driving_phases"] = [2, 3]
        self.assertRejects(ledger, "discharged its own deferral")

    def test_rejects_a_discharge_naming_a_phase_that_ran_before_this_one(self):
        # F14 reopened through the discharge fields. A closed row may name the
        # discharging phase's gates, so a phase 4 row that declares phase 3 its
        # discharger buys NxPhysicsCollisionTests with two fields it writes
        # itself -- and the gate-to-phase binding stays silent about it, which
        # is why the rule lives here and not there.
        ledger = self.discharged()
        ledger["phase"] = 4
        ledger["closed"][0]["driving_phases"] = [3]
        gate_errors = [error for error in self.errors(ledger, phase=4)
                       if "does not run when phase 4 is gated" in error
                       and "phys_fn_000001" in error]
        self.assertEqual(gate_errors, [], "the gate binding was supposed to be bought")
        self.assertRejects(ledger, "are not later than phase 4", phase=4)

    def test_rejects_a_discharge_that_does_not_carry_the_original_driving_phases(self):
        # discharged_by_phase on its own is unfalsifiable: there is nothing left
        # to check it against once the deferred entry is gone.
        ledger = self.discharged()
        del ledger["closed"][0]["driving_phases"]
        self.assertRejects(ledger, "driving_phases is missing")

    def test_rejects_driving_phases_on_a_closure_that_discharges_nothing(self):
        ledger = self.discharged()
        del ledger["closed"][0]["discharged_by_phase"]
        self.assertRejects(ledger, "discharged_by_phase is missing")

    def test_rejects_a_discharging_phase_that_is_not_a_phase_number(self):
        ledger = self.discharged()
        ledger["closed"][0]["discharged_by_phase"] = "three"
        self.assertRejects(ledger, "which is not a phase number")

    def test_rejects_discharge_driving_phases_that_are_not_phase_numbers(self):
        ledger = self.discharged()
        ledger["closed"][0]["driving_phases"] = [0]
        self.assertRejects(ledger, "are not phase numbers")

    def test_a_discharge_is_still_a_closure_and_needs_a_falsification(self):
        # The new field adds a claim; it does not remove one. A discharged row
        # that nobody aimed a mutation at is no more closed than any other.
        ledger = self.discharged()
        ledger["closed"][0]["falsification"]["detected"] = "mismatches=0"
        self.assertRejects(ledger, "not a non-zero mismatches count")

    def test_a_discharged_row_is_still_owned_by_the_phase_that_censused_it(self):
        # The row does not move. Putting it on the discharging phase's ledger
        # instead is what `stray` rejects, and that rule is unchanged.
        ledger = self.ledger()
        ledger["closed"].append({"id": "phys_fn_000004", "rva": "0x00001090",
                                 "proof": "oracle_differential_falsified",
                                 "gate": "NxPhysicsCollisionTests",
                                 "discharged_by_phase": 2, "driving_phases": [2],
                                 "falsification": {"mutation": "m", "detected": "mismatches=4"}})
        self.assertRejects(ledger, "names rows that are not phase 2: phys_fn_000004")

    def test_rejects_a_deferral_with_no_reason_the_programme_defines(self):
        ledger = self.ledger()
        ledger["deferred"][0]["reason"] = "later"
        self.assertRejects(ledger, "gives reason 'later'")

    def test_rejects_a_blocked_row_that_names_no_phase_to_inherit_it(self):
        ledger = self.ledger()
        ledger["deferred"][0]["driving_phases"] = []
        self.assertRejects(ledger, "blocked on no named phase")

    def test_rejects_a_row_blocked_on_a_phase_that_is_not_later(self):
        ledger = self.ledger()
        ledger["deferred"][0]["driving_phases"] = [2]
        self.assertRejects(ledger, "not later than 2")

    def test_rejects_driving_phases_that_are_not_phase_numbers(self):
        ledger = self.ledger()
        ledger["deferred"][0]["driving_phases"] = [999]
        self.assertRejects(ledger, "are not phase numbers")

    def test_rejects_driving_phases_that_disagree_with_the_blockers(self):
        # The two fields have to tell one story: a row waiting on a Phase 4 row
        # cannot claim it is waiting only on Phase 7.
        ledger = self.ledger()
        ledger["deferred"][0]["driving_phases"] = [7]
        self.assertRejects(ledger, "blocked on rows in phases 4, which it does not list")

    def test_rejects_a_blocked_row_that_names_neither_a_row_nor_a_type(self):
        ledger = self.ledger()
        ledger["deferred"][0]["blocked_on"] = []
        del ledger["deferred"][0]["blocked_on_type"]
        self.assertRejects(ledger, "names neither a blocking row nor a blocking type")

    def test_rejects_a_blocking_type_that_is_not_a_name(self):
        ledger = self.ledger()
        ledger["deferred"][0]["blocked_on_type"] = 7
        self.assertRejects(ledger, "names blocking type 7, which is not a name")

    def test_rejects_a_row_blocked_on_itself(self):
        ledger = self.ledger()
        ledger["deferred"][0]["blocked_on"] = ["phys_fn_000003"]
        self.assertRejects(ledger, "blocked on itself")

    def test_rejects_a_blocker_that_is_not_a_censused_row(self):
        ledger = self.ledger()
        ledger["deferred"][0]["blocked_on"] = ["phys_fn_999999"]
        self.assertRejects(ledger, "not a censused row")

    def test_rejects_a_ledger_that_claims_a_row_of_another_phase(self):
        ledger = self.ledger()
        ledger["closed"].append({"id": "phys_fn_000004", "rva": "0x00001090",
                                 "proof": "differential_falsified", "gate": "NxRealDifferential",
                                 "falsification": {"mutation": "m", "detected": "stdout_delta=1"}})
        ledger["counts"]["differential_falsified"] = 2
        self.assertRejects(ledger, "names rows that are not phase 2: phys_fn_000004")

    def test_rejects_a_ledger_whose_body_disagrees_with_its_file_name(self):
        self.assertRejects(self.ledger(), "file name says phase 3", phase=3)


class DischargePassedTests(unittest.TestCase):
    """A discharge counts only once the discharging phase has passed.

    The self-declared fields `_check_discharge` reads let phys_fn_002344 be
    re-closed with `discharged_by_phase: 5` while Phase 5 was pending, which is
    a row resting on a gate no passing run executes.
    """

    def ledgers(self, by):
        return {2: {"closed": [{"id": "phys_fn_001690", "discharged_by_phase": by,
                                "driving_phases": [3, 4]},
                               {"id": "phys_fn_000001"}]}}

    def program(self, **statuses):
        return {"phases": [{"phase": number, "status": statuses.get(f"p{number}", "pending")}
                           for number in range(1, 9)]}

    def test_accepts_a_discharge_by_a_phase_that_passed(self):
        self.assertEqual(validate_inventory.validate_discharge_passed(
            self.ledgers(3), self.program(p3="pass")), [])

    def test_rejects_a_discharge_by_a_pending_phase(self):
        errors = validate_inventory.validate_discharge_passed(self.ledgers(5),
                                                              self.program(p3="pass"))
        self.assertIn("discharged by phase 5, which program.json records as 'pending'",
                      "\n".join(errors))

    def test_rejects_a_discharge_by_a_failing_phase(self):
        errors = validate_inventory.validate_discharge_passed(self.ledgers(3),
                                                              self.program(p3="fail"))
        self.assertIn("records as 'fail'", "\n".join(errors))

    def test_rejects_a_discharge_by_a_phase_program_json_does_not_declare(self):
        errors = validate_inventory.validate_discharge_passed(self.ledgers(3), {"phases": []})
        self.assertIn("records as None", "\n".join(errors))

    def test_a_closure_that_discharges_nothing_is_not_touched(self):
        ledgers = {2: {"closed": [{"id": "phys_fn_000001"}]}}
        self.assertEqual(validate_inventory.validate_discharge_passed(ledgers, self.program()), [])


class ProgramRecordTests(unittest.TestCase):
    """Every number program.json states is recomputed from the census it quotes.

    program.json is what a later phase reads to decide what is already done, and
    nothing compared it to inventory.json: every phase's counters went stale at
    once and no gate said so.
    """

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        # program.json records plan paths from the repository root, which is
        # three levels above the evidence root.
        self.root = Path(self.directory.name) / "docs" / "reconstruction" / "novodex-physics"
        (self.root / "gates").mkdir(parents=True)
        (self.root / "inventory.json").write_text("{}", encoding="utf-8")
        plans = Path(self.directory.name) / "docs" / "superpowers" / "plans"
        plans.mkdir(parents=True)
        for name in ("phase1.md", "phase2.md"):
            (plans / name).write_text("# plan\n", encoding="utf-8")
        self.addCleanup(self.directory.cleanup)

    def closure(self):
        """A ledger closing one function row of phase 2 and deferring the rest."""
        return {
            "closed": [{"id": "phys_fn_000001"}],
            "deferred": [{"id": "phys_fn_000002"}, {"id": "phys_data_000001"}],
        }

    def closed_phase_two(self, inventory, program):
        inventory["phases"][1]["status"] = "closed"
        phase = program["phases"][1]
        phase["status"] = "pass"
        phase["implementation_commit"] = "a" * 40
        phase["evidence_commit"] = "d" * 40
        phase["gate_artifact"] = "gates/phase2-closure.json"
        (self.root / "gates" / "phase2-closure.json").write_text("{}", encoding="utf-8")
        phase["closed_functions"] = 1
        phase["closed_data_objects"] = 0
        phase["remaining_functions"] = 1
        phase["remaining_data_objects"] = 1
        return {2: self.closure()}

    def errors(self, inventory, program, closures):
        return validate_inventory.validate_program(inventory, program, closures, self.root)

    def assertRejects(self, inventory, program, closures, message):
        errors = self.errors(inventory, program, closures)
        self.assertTrue(any(re.search(message, error) for error in errors),
                        f"{message!r} not in {errors}")

    def test_accepts_a_programme_that_matches_the_census(self):
        self.assertEqual(self.errors(minimal_inventory(), minimal_program(), {}), [])

    def test_accepts_a_closed_phase_whose_counts_come_from_its_ledger(self):
        inventory, program = minimal_inventory(), minimal_program()
        closures = self.closed_phase_two(inventory, program)
        self.assertEqual(self.errors(inventory, program, closures), [])

    def test_rejects_an_owned_count_the_census_does_not_support(self):
        program = minimal_program()
        program["phases"][1]["owned_functions"] = 7
        self.assertRejects(minimal_inventory(), program, {},
                           "claims owned_functions=7 but the census assigns it 2")

    def test_rejects_an_owned_data_count_the_census_does_not_support(self):
        program = minimal_program()
        program["phases"][1]["owned_data_objects"] = 40
        self.assertRejects(minimal_inventory(), program, {},
                           "claims owned_data_objects=40 but the census assigns it 1")

    def test_rejects_a_closed_count_the_ledger_does_not_support(self):
        inventory, program = minimal_inventory(), minimal_program()
        closures = self.closed_phase_two(inventory, program)
        program["phases"][1]["closed_functions"] = 2
        self.assertRejects(inventory, program, closures,
                           "claims closed_functions=2 but the census and its closure ledger give 1")

    def test_rejects_a_remaining_count_that_does_not_follow(self):
        inventory, program = minimal_inventory(), minimal_program()
        closures = self.closed_phase_two(inventory, program)
        program["phases"][1]["remaining_functions"] = 0
        self.assertRejects(inventory, program, closures, "claims remaining_functions=0")

    def test_rejects_a_status_the_inventory_does_not_agree_with(self):
        # The two files spell a finished phase differently; that is the only
        # difference between them the programme allows.
        program = minimal_program()
        program["phases"][1]["status"] = "pass"
        self.assertRejects(minimal_inventory(), program, {},
                           "says 'pass' but inventory.json says the phase is 'pending'")

    def test_rejects_a_phase_named_differently_in_the_two_files(self):
        program = minimal_program()
        program["phases"][1]["name"] = "SDK core"
        self.assertRejects(minimal_inventory(), program, {}, "but inventory.json names it")

    def test_rejects_a_phase_the_inventory_does_not_declare(self):
        program = minimal_program()
        program["phases"][1]["phase"] = 3
        self.assertRejects(minimal_inventory(), program, {},
                           r"declares phases \[1, 3\] but inventory.json declares \[1, 2\]")

    def test_rejects_a_programme_pinning_a_different_oracle(self):
        program = minimal_program()
        program["oracle"]["sha256"] = "ff" + "0" * 62
        self.assertRejects(minimal_inventory(), program, {},
                           "program.oracle does not pin what inventory.pins.oracle pins")

    def test_rejects_a_programme_pinning_a_different_header_root(self):
        program = minimal_program()
        program["header_root"]["sdk_version"] = "2.1.2.5000"
        self.assertRejects(minimal_inventory(), program, {},
                           "program.header_root does not pin what inventory.pins.public_headers")

    def test_rejects_an_unexpected_programme_key(self):
        program = minimal_program()
        program["closeout"] = "closeout.md"
        self.assertRejects(minimal_inventory(), program, {}, "unexpected key 'closeout'")

    def test_rejects_a_phase_passing_with_no_ledger_to_account_for_its_rows(self):
        inventory, program = minimal_inventory(), minimal_program()
        inventory["phases"][1]["status"] = "closed"
        program["phases"][1].update(status="pass", implementation_commit="a" * 40,
                                    evidence_commit="d" * 40, gate_artifact="inventory.json")
        self.assertRejects(inventory, program, {},
                           "passes while owning 2 functions and 1 data objects")

    def test_rejects_a_passing_phase_that_records_no_evidence_commit(self):
        inventory, program = minimal_inventory(), minimal_program()
        closures = self.closed_phase_two(inventory, program)
        program["phases"][1]["evidence_commit"] = None
        self.assertRejects(inventory, program, closures, "passes without recording evidence_commit")

    def test_rejects_a_closed_code_row_with_no_implementation_commit(self):
        # Closing one means a mutation was aimed at a reconstruction, so there
        # is a tree it was measured against.
        inventory, program = minimal_inventory(), minimal_program()
        closures = self.closed_phase_two(inventory, program)
        program["phases"][1]["implementation_commit"] = None
        self.assertRejects(inventory, program, closures,
                           "closes 1 function rows without recording the implementation_commit")

    def test_an_evidence_only_phase_may_pass_without_an_implementation(self):
        inventory, program = minimal_inventory(), minimal_program()
        closures = self.closed_phase_two(inventory, program)
        closures[2]["closed"] = []
        closures[2]["deferred"].append({"id": "phys_fn_000001"})
        program["phases"][1].update(implementation_commit=None, closed_functions=0,
                                    remaining_functions=2)
        self.assertEqual(self.errors(inventory, program, closures), [])

    def test_rejects_a_gate_artifact_that_is_not_a_file(self):
        inventory, program = minimal_inventory(), minimal_program()
        closures = self.closed_phase_two(inventory, program)
        program["phases"][1]["gate_artifact"] = "gates/phase2-nothing.json"
        self.assertRejects(inventory, program, closures, "which is not a file")

    def test_rejects_a_gate_artifact_that_leaves_the_phase_record_unread(self):
        inventory, program = minimal_inventory(), minimal_program()
        closures = self.closed_phase_two(inventory, program)
        (self.root / "gates" / "phase2.json").write_text("{}", encoding="utf-8")
        self.assertRejects(inventory, program, closures,
                           "while a gates/phase2.json record sits unread beside it")

    def test_rejects_a_phase_naming_a_plan_that_does_not_exist(self):
        # A phase whose plan file is gone is a phase nobody can execute, and the
        # plan is the other end of every escalation it inherits.
        program = minimal_program()
        program["phases"][1]["plan"] = "docs/superpowers/plans/gone.md"
        self.assertRejects(minimal_inventory(), program, {}, "which is not a file")

    def test_rejects_a_phase_that_states_no_gate(self):
        program = minimal_program()
        program["phases"][1]["gate"] = "  "
        self.assertRejects(minimal_inventory(), program, {}, "states no gate")


class RowStateCeilingTests(unittest.TestCase):
    """No row stands above `reconstructed` without a ledger entry putting it there.

    The floors in validate_closure left the ceiling open: every row could be
    raised to `closed` by editing one field of one file, with no ledger change
    and nothing objecting.
    """

    def inventory(self, state, identifier="phys_fn_000001"):
        data = minimal_inventory()
        for row in data["functions"] + data["data_objects"]:
            if row["id"] == identifier:
                row["state"] = state
        return data

    def closures(self, phase=2, proof="differential_falsified"):
        return {phase: {"closed": [{"id": "phys_fn_000001", "proof": proof}],
                        "deferred": [{"id": "phys_fn_000002"}]}}

    def assertRejects(self, inventory, closures, message):
        errors = validate_inventory.validate_row_states(inventory, closures)
        self.assertTrue(any(re.search(message, error) for error in errors),
                        f"{message!r} not in {errors}")

    def test_accepts_a_gated_row_its_ledger_closes(self):
        self.assertEqual(validate_inventory.validate_row_states(
            self.inventory("dynamically_gated"), self.closures()), [])

    def test_accepts_an_ungated_row_at_the_ceiling(self):
        self.assertEqual(validate_inventory.validate_row_states(
            self.inventory("reconstructed"), {}), [])

    def test_rejects_a_row_closed_by_a_phase_that_is_not_the_full_census_audit(self):
        # The whole point of leaving Phase 2's rows below `closed`: entry-only
        # deltas do not establish that a row is finished.
        self.assertRejects(self.inventory("closed"), self.closures(),
                           "the terminal state is the full-census audit's to grant")

    def test_the_full_census_audit_may_grant_the_terminal_state(self):
        # Bounded, not forbidden: Phase 8 has to be able to close these rows.
        data = self.inventory("closed")
        data["phases"] = [{"phase": number, "name": str(number), "status": "pending"}
                          for number in range(1, 9)]
        for row in data["functions"] + data["data_objects"]:
            row["phase"] = 8 if row["id"] == "phys_fn_000001" else row["phase"]
        self.assertEqual(validate_inventory.validate_row_states(data, self.closures(phase=8)), [])

    def test_rejects_a_deferred_row_raised_above_the_ungated_ceiling(self):
        self.assertRejects(self.inventory("statically_reviewed", "phys_fn_000002"),
                           self.closures(), "with no closure ledger closing it")

    def test_rejects_a_row_no_ledger_mentions_at_all(self):
        self.assertRejects(self.inventory("dynamically_gated"), {},
                           "with no closure ledger closing it")

    def test_notices_a_phase_declared_after_the_pinned_full_census_audit(self):
        data = self.inventory("reconstructed")
        data["phases"].append({"phase": 9, "name": "after", "status": "pending"})
        self.assertRejects(data, {}, "the programme has changed shape")


class DifferentialExclusionTests(unittest.TestCase):
    """What a phase record says its differential left out is read from the runner."""

    RUNNER = Path(validate_inventory.__file__).resolve().parent / "run_differential.ps1"

    def read(self, text):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "run_differential.ps1"
            path.write_text(text, encoding="utf-8")
            return validate_inventory.read_differential_exclusions(path)

    def test_reads_the_live_exclusion_list(self):
        self.assertEqual(validate_inventory.read_differential_exclusions(self.RUNNER),
                         ["pair_directory=", "loaded module=", "modules ", "imports "])

    def test_reports_nothing_when_the_runner_stops_naming_one_list(self):
        # Two lists or none means the record has nothing to be checked against,
        # which the phase record reports rather than silently accepting.
        self.assertIsNone(self.read("$normalized = @($lines)\n"))


class EscalationLinkageTests(unittest.TestCase):
    """An escalation and the plan that inherits it pin each other, both ways."""

    def records(self, inherited_by=(8,)):
        return {2: {"escalations": [{"id": "one_ulp", "inherited_by": list(inherited_by)}]}}

    def test_accepts_an_escalation_its_inheritor_names(self):
        self.assertEqual(validate_inventory.validate_escalations(
            self.records(), {8: {("one_ulp", 2)}}), [])

    def test_rejects_an_escalation_the_inheriting_plan_does_not_name(self):
        self.assertIn("whose plan does not name it", "\n".join(
            validate_inventory.validate_escalations(self.records(), {8: set()})))

    def test_rejects_an_escalation_inherited_by_a_phase_with_no_plan(self):
        self.assertIn("which has no plan to record it in", "\n".join(
            validate_inventory.validate_escalations(self.records(), {})))

    def test_rejects_a_plan_obligation_the_record_deleted(self):
        # A record is the only place an escalation lives, so the plan is what
        # stops one being deleted out of the programme's knowledge.
        self.assertIn("which does not record it", "\n".join(
            validate_inventory.validate_escalations({2: {"escalations": []}},
                                                    {8: {("one_ulp", 2)}})))

    def test_reads_the_obligations_a_plan_takes_on(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "plan.md"
            path.write_text("- [ ] Discharge the `one_ulp` escalation in `gates/phase2.json`.\n",
                            encoding="utf-8")
            self.assertEqual(validate_inventory.read_plan_escalations(path), {("one_ulp", 2)})


class PlanPathTests(unittest.TestCase):
    """Where a phase plan is looked for, given where the evidence tree is."""

    def test_resolves_a_relative_evidence_root(self):
        # `validate_inventory.py inventory.json` run from the evidence directory
        # gave this a root of `.`, whose `parents[2]` raised an IndexError: a
        # traceback that reads exactly like a validation failure and is not one.
        # The production runner always passes an absolute path, so it was never
        # seen from the gate.
        self.assertEqual(
            validate_inventory.plan_path(Path("docs/reconstruction/novodex-physics"),
                                         "docs/superpowers/plans/p.md"),
            Path.cwd().resolve() / "docs/superpowers/plans/p.md")

    def test_reports_a_root_too_shallow_to_sit_in_a_repository(self):
        # Not a crash and not a silent pass: no path, so the caller says the
        # plan is not a file, which is a stated defect.
        self.assertIsNone(validate_inventory.plan_path(Path(Path.cwd().anchor), "p.md"))


class ClosureEvidenceTests(unittest.TestCase):
    """A closed row's count has to be readable in the file the ledger publishes it in.

    Everything else about a closure is checked across four documents. The
    measurement itself was checked against nothing, so an already-closed row
    could be moved onto a different mutation -- a documented GREEN one, say --
    in a single file with no detector.
    """

    EVIDENCE = "evidence/phase4-falsification.md"

    def ledger(self, detected="mismatches=10", **overrides):
        ledger = {"phase": 4, "evidence_file": self.EVIDENCE,
                  "closed": [{"id": "phys_fn_002045",
                              "falsification": {"mutation": "mutation J", "detected": detected}}]}
        ledger.update(overrides)
        return ledger

    def errors(self, ledger, published, phase=4):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "evidence").mkdir()
            (root / self.EVIDENCE).write_text(published, encoding="utf-8")
            return validate_inventory.validate_closure_evidence(ledger, phase, root)

    def test_accepts_a_row_whose_count_is_on_its_own_line(self):
        self.assertEqual(self.errors(
            self.ledger(), "| `phys_fn_002045` `0x000505f0`+69 | J | 10 |\n"), [])

    def test_rejects_a_row_moved_onto_a_count_the_evidence_does_not_carry(self):
        # The defect this exists for: the row stays closed, the gate, the proof
        # kind and the counts all still agree, and only the measurement moved.
        errors = self.errors(self.ledger("mismatches=7"),
                             "| `phys_fn_002045` `0x000505f0`+69 | J | 10 |\n")
        self.assertIn("recording different measurements", "\n".join(errors))

    def test_rejects_a_closed_row_the_evidence_file_does_not_name(self):
        errors = self.errors(self.ledger(), "| `phys_fn_002047` | A | 10 |\n")
        self.assertIn("is not named in", "\n".join(errors))

    def test_does_not_match_a_count_inside_a_longer_number(self):
        errors = self.errors(self.ledger("mismatches=234"),
                             "| `phys_fn_002045` `0x00050640`+2340 | J | 10 |\n")
        self.assertIn("recording different measurements", "\n".join(errors))

    def test_a_static_proof_has_no_count_to_bind(self):
        self.assertEqual(self.errors(self.ledger("check_failed the guard holds"),
                                     "`phys_fn_002045` drops the guard\n"), [])

    def test_a_phase_2_or_3_ledger_naming_no_evidence_file_is_unchanged(self):
        # Phases 2 and 3 published their measurements before stable IDs were
        # written into the evidence, so they name no file and bind nothing.
        ledger = self.ledger()
        del ledger["evidence_file"]
        for phase in (2, 3):
            self.assertEqual(self.errors(ledger, "", phase=phase), [])

    def test_rejects_a_later_ledger_that_drops_its_evidence_file(self):
        # The deletion attack: drop the key and move a row onto an invented
        # count in the same edit. With the key optional that validated.
        ledger = self.ledger("mismatches=7")
        del ledger["evidence_file"]
        for phase in (4, 6, 7):
            self.assertIn("names no evidence_file",
                          "\n".join(self.errors(ledger, "", phase=phase)))

    def test_a_later_ledger_that_closes_nothing_needs_no_evidence_file(self):
        self.assertEqual(self.errors({"phase": 5, "closed": []}, "", phase=5), [])

    def test_rejects_an_evidence_file_that_is_not_there(self):
        ledger = self.ledger(evidence_file="evidence/nothing.md")
        self.assertIn("cannot be read", "\n".join(self.errors(ledger, "")))


class PhaseRecordTests(unittest.TestCase):
    """A phase gate record may not disagree with the ledger it sits beside."""

    EXCLUSIONS = ["loaded module="]

    def record(self, **overrides):
        record = {
            "schema_version": 1, "phase": 2, "name": "shared runtime", "status": "pass",
            "closure_ledger": "gates/phase2-closure.json",
            "implementation_commit": "a" * 40, "evidence_commit": "d" * 40,
            "note": "", "counts": {"closed": 1, "deferred": 2},
            "gate_sequence": [{"name": "validate_inventory", "command": "python validate.py",
                               "result": "inventory=pass"}],
            "excluded_from_comparison": list(self.EXCLUSIONS),
            "escalations": [{"id": "one_ulp", "summary": "one ULP apart",
                             "reproduction": "run the sphere replay and read line 79",
                             "evidence": "evidence/phase2-fluid-visualization.md",
                             "inherited_by": [8]}],
        }
        record.update(overrides)
        return record

    def program_phase(self, **overrides):
        phase = {"status": "pass", "name": "shared runtime",
                 "implementation_commit": "a" * 40, "evidence_commit": "d" * 40}
        phase.update(overrides)
        return phase

    def totals(self):
        return validate_inventory._ledger_totals(
            {"closed": [{"id": "phys_fn_000001"}],
             "deferred": [{"id": "phys_fn_000002"}, {"id": "phys_data_000001"}]})

    def errors(self, record, phase=2, program=None, totals=True, exclusions=True):
        return validate_inventory.validate_phase_record(
            record, phase, self.program_phase() if program is None else program,
            self.totals() if totals else None,
            list(self.EXCLUSIONS) if exclusions else None)

    def assertRejects(self, record, message, **kwargs):
        errors = self.errors(record, **kwargs)
        self.assertTrue(any(re.search(message, error) for error in errors),
                        f"{message!r} not in {errors}")

    def test_accepts_a_record_that_matches_its_ledger(self):
        self.assertEqual(self.errors(self.record()), [])

    def test_rejects_counts_the_ledger_does_not_support(self):
        self.assertRejects(self.record(counts={"closed": 59, "deferred": 2}),
                           "do not match its closure ledger")

    def test_rejects_a_status_program_json_does_not_agree_with(self):
        self.assertRejects(self.record(), "says status='pass' but program.json says 'pending'",
                           program=self.program_phase(status="pending"))

    def test_rejects_a_name_program_json_does_not_agree_with(self):
        self.assertRejects(self.record(), "says name='shared runtime' but program.json says",
                           program=self.program_phase(name="SDK core"))

    def test_rejects_commits_program_json_does_not_agree_with(self):
        # The record's note leans on both; neither was cross-checked.
        for field in ("implementation_commit", "evidence_commit"):
            with self.subTest(field=field):
                self.assertRejects(self.record(), f"says {field}=",
                                   program=self.program_phase(**{field: "0" * 40}))

    def test_rejects_a_record_whose_body_disagrees_with_its_file_name(self):
        self.assertRejects(self.record(), "file name says phase 3", phase=3)

    def test_rejects_a_record_with_no_closure_ledger_beside_it(self):
        self.assertRejects(self.record(), "so nothing accounts for the rows it reports on",
                           totals=False)

    def test_rejects_a_record_that_runs_no_command(self):
        # A record with the right two numbers in it and no gate side is a file,
        # not a gate.
        self.assertRejects(self.record(gate_sequence=[]), "runs no command")

    def test_rejects_a_gate_step_that_reports_nothing(self):
        record = self.record()
        record["gate_sequence"][0]["result"] = "   "
        self.assertRejects(record, r"gate_sequence\[0\] records no result")

    def test_rejects_an_exclusion_list_the_runner_does_not_support(self):
        self.assertRejects(self.record(excluded_from_comparison=[]),
                           "but run_differential.ps1 excludes")

    def test_rejects_a_record_whose_exclusion_list_cannot_be_checked(self):
        self.assertRejects(self.record(), "could not be read back out of run_differential.ps1",
                           exclusions=False)

    def test_rejects_a_record_naming_another_phases_ledger(self):
        self.assertRejects(self.record(closure_ledger="gates/phase3-closure.json"),
                           "rather than its own phase's")

    def test_rejects_an_escalation_with_no_reproduction(self):
        # An escalation whose text is a conclusion is one nobody can re-run.
        record = self.record()
        record["escalations"][0]["reproduction"] = "  "
        self.assertRejects(record, "records no reproduction")

    def test_rejects_an_escalation_with_no_evidence(self):
        record = self.record()
        record["escalations"][0]["evidence"] = None
        self.assertRejects(record, "records no evidence")

    def test_rejects_an_escalation_nobody_inherits(self):
        record = self.record()
        record["escalations"][0]["inherited_by"] = []
        self.assertRejects(record, "which are not phase numbers")

    def test_rejects_an_escalation_inherited_by_an_earlier_phase(self):
        record = self.record()
        record["escalations"][0]["inherited_by"] = [1]
        self.assertRejects(record, "is inherited by phase 1, which is not later than 2")

    def test_rejects_an_unexpected_record_key(self):
        self.assertRejects(self.record(closeout="closeout.md"), "unexpected key 'closeout'")


class GateTargetRegistryTests(unittest.TestCase):
    """A gate name is checked against the registry the phase gate actually runs."""

    REGISTRY = Path(validate_inventory.__file__).resolve().parent / "gate_targets.ps1"

    def read(self, text):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "gate_targets.ps1"
            path.write_text(text, encoding="utf-8")
            return validate_inventory.read_gate_targets(path)

    def test_reads_the_three_registries_apart(self):
        targets = validate_inventory.read_gate_targets(self.REGISTRY)
        self.assertIn("NxPhysicsCoreClusterTests", targets["differential"])
        self.assertIn("NxPhysicsInternalTests", targets["static_proof"])
        self.assertIn("NxPhysicsCollisionTests", targets["oracle_differential"])
        self.assertNotIn("2", targets["differential"])
        # The separations the split lists exist to keep.
        self.assertNotIn("NxPhysicsInternalTests", targets["differential"])
        self.assertNotIn("NxPhysicsCollisionTests", targets["differential"])
        self.assertNotIn("NxPhysicsCollisionTests", targets["static_proof"])

    def test_the_registry_binds_each_target_to_the_phase_that_runs_it(self):
        # The flat name sets say a gate exists; `by_phase` says which phase's
        # gate run executes it, which is what stops a row closing on a target
        # that never runs when its own phase is gated.
        targets = validate_inventory.read_gate_targets(self.REGISTRY)
        oracle = targets["by_phase"]["oracle_differential"]
        self.assertEqual(oracle[3], {"NxPhysicsCollisionTests"})
        self.assertEqual(oracle[4], {"NxPhysicsAssetTests", "NxPhysicsThirdPartyTests"})
        self.assertEqual(oracle[5], {"NxPhysicsObjectLayoutTests"})
        self.assertNotIn("NxPhysicsCollisionTests", oracle[4])
        self.assertNotIn("NxPhysicsObjectLayoutTests", oracle[3])
        self.assertEqual(targets["by_phase"]["static_proof"][2], {"NxPhysicsInternalTests"})
        self.assertIn("NxPhysicsGeometryTests", targets["by_phase"]["differential"][3])
        self.assertNotIn("NxPhysicsGeometryTests", targets["by_phase"]["differential"].get(2, ()))

    def test_a_commented_out_target_is_not_registered(self):
        # It fails open: a target nobody runs would have vouched for a closure.
        targets = self.read("$NxPhaseTestTargets = [ordered] @{\n"
                            "    '2' = @('Live')  # @('GhostTarget')\n"
                            "}\n")
        self.assertEqual(targets["differential"], {"Live"})

    def test_an_indented_closing_brace_does_not_merge_the_registries(self):
        # One space in front of the brace used to fold the static-proof list
        # into the differential one, which is the merge the split exists to stop.
        targets = self.read("$NxPhaseTestTargets = [ordered] @{\n"
                            "    '2' = @('Live')\n"
                            " }\n"
                            "$NxPhaseStaticProofTargets = [ordered] @{\n"
                            "    '2' = @('WhiteBox')\n"
                            "}\n")
        self.assertNotIn("WhiteBox", targets["differential"])
        self.assertIn("WhiteBox", targets["static_proof"])


def ruling_instruction(rva, mnemonic, operands, indirect=False, target=None):
    return {"rva": f"0x{rva:08x}", "mnemonic": mnemonic, "operands": operands,
            "indirect": indirect, "flow": "call" if mnemonic == "call" else "sequential",
            "target_rva": None if target is None else f"0x{target:08x}"}


def ruling_row(identifier, rva, size, phase, provenance, notes=""):
    return {"id": identifier, "rva": f"0x{rva:08x}", "size": size, "phase": phase,
            "phase_provenance": provenance, "notes": notes}


class ShapeRulingTests(unittest.TestCase):
    """The shape ruling is checked against the oracle, then the census against it.

    One table T at 0x3000 holds A (object model) and B (collision). C constructs
    it. M is the matrix row: it stores X, a pipeline row that calls through slot 1
    on a receiver `receivers` types as a T. H is a helper only A calls, and A2 is a
    continuation of A. B's slot is collision because X dispatches it; A's is object
    model because nothing in the pipeline does.
    """

    SHA = "4b" + "0" * 62

    def fixture(self):
        inventory = {"pins": {"oracle": {"sha256": self.SHA}}, "exports": [], "functions": [
            ruling_row("phys_fn_000001", 0x1000, 8, 5, "slot_ruling"),
            ruling_row("phys_fn_000002", 0x1008, 8, 5, "slot_ruling",
                       "continuation of the entry at 0x00001000"),
            ruling_row("phys_fn_000003", 0x1010, 16, 3, "slot_ruling"),
            ruling_row("phys_fn_000004", 0x1020, 16, 5, "slot_ruling"),
            ruling_row("phys_fn_000005", 0x1030, 16, 3, "callers"),
            ruling_row("phys_fn_000006", 0x1040, 16, 5, "slot_ruling"),
            ruling_row("phys_fn_000007", 0x1050, 16, 2, "callers"),
        ]}
        ruling = {
            "schema_version": 1, "oracle_sha256": self.SHA, "rule": "by slot",
            "phase_of": {"collision": 3, "object_model": 5},
            "pipeline": {"matrix": "phys_fn_000007", "phases": [3, 4], "bound": "3 and 4",
                         "worked_examples": []},
            "tables": [{"class": "T", "rva": "0x00003000", "constructor": "phys_fn_000004",
                        "vptr_store": "0x00001024", "slots": ["object_model", "collision"]}],
            "receivers": [{"site": "0x00001034", "row": "phys_fn_000005", "receiver": ["T"],
                           "note": "the fixture's T"}],
            "members": [
                {"id": "phys_fn_000004", "rva": "0x00001020", "basis": "constructor",
                 "ruling": "object_model", "site": "0x00001024"},
                {"id": "phys_fn_000006", "rva": "0x00001040", "basis": "helper",
                 "ruling": "object_model",
                 "callers": [{"site": "0x00001004", "row": "phys_fn_000001"}]}],
            "exceptions": []}
        pointers = {0x3000: 0x1000, 0x3004: 0x1010}
        instructions = {
            0x1004: ruling_instruction(0x1004, "call", "0x10001040", target=0x1040),
            0x1024: ruling_instruction(0x1024, "mov", "dword ptr [esi], 0x10003000"),
            0x1032: ruling_instruction(0x1032, "mov", "eax, dword ptr [ecx]"),
            0x1034: ruling_instruction(0x1034, "call", "dword ptr [eax + 4]", indirect=True),
            0x1054: ruling_instruction(0x1054, "mov", "dword ptr [edx + 8], 0x10001030"),
        }
        return inventory, ruling, pointers, instructions

    def errors(self, mutate=None, spans=None):
        inventory, ruling, pointers, instructions = self.fixture()
        if mutate:
            mutate(inventory, ruling, pointers, instructions)
        return validate_inventory.validate_shape_ruling(inventory, ruling, pointers, instructions,
                                                        [], 0x10000000, frozenset(), spans)

    def assertRejects(self, mutate, pattern, spans=None):
        errors = self.errors(mutate, spans)
        self.assertTrue(any(re.search(pattern, error) for error in errors),
                        f"{pattern!r} not in {errors}")

    @staticmethod
    def row(inventory, identifier):
        return next(row for row in inventory["functions"] if row["id"] == identifier)

    def test_accepts_a_census_that_agrees_with_the_ruling(self):
        self.assertEqual(self.errors(), [])

    def test_the_resolver_gives_every_named_row_one_phase(self):
        _, ruling, pointers, _ = self.fixture()
        phases, slots, errors = validate_inventory.resolve_shape_ruling(ruling, pointers)
        self.assertEqual(errors, [])
        self.assertEqual(phases, {0x1000: 5, 0x1010: 3, 0x1020: 5, 0x1040: 5})

    def test_the_pipeline_is_walked_from_the_matrix(self):
        inventory, ruling, pointers, instructions = self.fixture()
        closure, dispatches, dispatched, interfaces, errors = validate_inventory.collision_pipeline(
            ruling, inventory["functions"], instructions, [], pointers, frozenset(), 0x10000000)
        self.assertEqual(errors, [])
        self.assertEqual(sorted(closure), [0x1010, 0x1030])
        self.assertEqual(dispatches, {0x1034: (0x1030, 1, ("T",), "site", None)})
        self.assertEqual(dict(dispatched), {("T", 1): {0x1034}})
        self.assertEqual(interfaces, {"T": "T"})

    def test_rejects_a_ruled_row_moved_to_another_phase(self):
        self.assertRejects(lambda i, r, p, s: self.row(i, "phys_fn_000001").__setitem__("phase", 3),
                           r"'phys_fn_000001' is placed by the shape ruling at phase 5")

    def test_rejects_a_ruled_row_relabelled_to_a_weaker_rule(self):
        self.assertRejects(
            lambda i, r, p, s: self.row(i, "phys_fn_000003").__setitem__(
                "phase_provenance", "layout_adjacency"),
            r"'phys_fn_000003' is placed by the shape ruling at phase 3 with provenance "
            r"'slot_ruling', but the census records phase 3 'layout_adjacency'")

    def test_rejects_a_continuation_left_behind_by_its_entry(self):
        self.assertRejects(lambda i, r, p, s: self.row(i, "phys_fn_000002").__setitem__("phase", 3),
                           r"'phys_fn_000002' is placed by the shape ruling")

    def test_a_continuation_with_a_note_appended_is_still_a_continuation(self):
        # The review found three continuation rows validated as entries because text
        # had been appended to their notes and the pattern was anchored at the end.
        self.assertRejects(
            lambda i, r, p, s: self.row(i, "phys_fn_000002").update(
                phase=3, notes="continuation of the entry at 0x00001000 P4 Task 3: retyped"),
            r"'phys_fn_000002' is placed by the shape ruling")
        self.assertEqual(validate_inventory._entry_rva(
            {"rva": "0x00001008", "notes": "continuation of the entry at 0x00001000 later text"}),
            0x1000)

    def test_rejects_an_unruled_row_claiming_the_ruling(self):
        self.assertRejects(
            lambda i, r, p, s: self.row(i, "phys_fn_000005").__setitem__(
                "phase_provenance", "slot_ruling"),
            r"'phys_fn_000005' claims 'slot_ruling', but the shape ruling does not place it")

    def test_rejects_a_collision_slot_the_pipeline_never_dispatches(self):
        # The receiver is typed as nothing the tables hold, so X dispatches no T slot.
        self.assertRejects(lambda i, r, p, s: r["receivers"][0].__setitem__("receiver", []),
                           r"rules T slot 1 collision, but the collision pipeline never "
                           r"dispatches it")

    def test_rejects_an_object_model_slot_the_pipeline_dispatches(self):
        # The shape of the box-hull finding: the pipeline calls through a slot the
        # table calls object model.
        self.assertRejects(
            lambda i, r, p, s: s[0x1034].__setitem__("operands", "dword ptr [eax]"),
            r"rules T slot 0 object_model, but the collision pipeline dispatches it at 0x00001034")

    def test_rejects_a_dispatch_on_an_untyped_receiver(self):
        self.assertRejects(lambda i, r, p, s: r.__setitem__("receivers", []),
                           r"dispatches slot 1 at 0x00001034 in 'phys_fn_000005' on a receiver "
                           r"the ruling does not type")

    def test_rejects_typing_a_site_the_pipeline_does_not_reach(self):
        def mutate(i, r, p, s):
            r["receivers"].append({"site": "0x00001038", "row": "phys_fn_000005",
                                   "receiver": ["T"], "note": "not a dispatch"})
        self.assertRejects(mutate, r"types the receiver at 0x00001038, which is not a dispatch")

    def test_a_slot_targets_this_is_typed_from_the_tables(self):
        # B dispatches slot 1 on its own this; B is T's slot 1, so the receiver is a T
        # with no declaration, and a declaration for it is refused.
        def mutate(i, r, p, s):
            s[0x1012] = ruling_instruction(0x1012, "mov", "eax, dword ptr [ecx]")
            s[0x1014] = ruling_instruction(0x1014, "call", "dword ptr [eax]", indirect=True)
        self.assertRejects(mutate, r"rules T slot 0 object_model, but the collision pipeline "
                                   r"dispatches it at 0x00001014")

        def declared(i, r, p, s):
            mutate(i, r, p, s)
            s[0x1014]["operands"] = "dword ptr [eax + 4]"
            r["receivers"].append({"site": "0x00001014", "row": "phys_fn_000003",
                                   "receiver": [], "note": "wrong"})
        self.assertRejects(declared, r"types 0x00001014, whose receiver is the incoming this")

    def test_a_receiver_can_be_typed_by_the_row_that_returns_it(self):
        def mutate(i, r, p, s):
            i["functions"].append(ruling_row("phys_fn_000008", 0x1060, 16, 2, "callers"))
            s[0x1036] = ruling_instruction(0x1036, "call", "0x10001060", target=0x1060)
            s[0x1038] = ruling_instruction(0x1038, "mov", "edx, dword ptr [eax]")
            s[0x103a] = ruling_instruction(0x103a, "call", "dword ptr [edx]", indirect=True)
            r["receivers"].append({"returned_by": "phys_fn_000008", "receiver": [],
                                   "note": "an allocator"})
        self.assertEqual(self.errors(mutate), [])

        def unused(i, r, p, s):
            mutate(i, r, p, s)
            del s[0x103a]
        self.assertRejects(unused, r"types what 0x00001060 returns, and no dispatch")

    def test_the_bound_ends_the_pipeline_at_another_phases_row(self):
        # X on Phase 5 is outside the pipeline, so nothing dispatches slot 1.
        self.assertRejects(lambda i, r, p, s: self.row(i, "phys_fn_000005").__setitem__("phase", 5),
                           r"rules T slot 1 collision, but the collision pipeline never dispatches")

    def test_rejects_a_bound_that_is_not_the_decision(self):
        self.assertRejects(lambda i, r, p, s: r["pipeline"].__setitem__("phases", [3]),
                           r"pipeline phases \[3\] are not the decision")

    def test_a_worked_example_is_checked_against_the_pipeline(self):
        def example(in_pipeline):
            def mutate(i, r, p, s):
                r["pipeline"]["worked_examples"] = [
                    {"class": "T", "slot": 1, "verdict": "collision", "note": "x",
                     "path": [{"row": "phys_fn_000005", "site": "0x00001034",
                               "in_pipeline": in_pipeline}]}]
            return mutate
        self.assertEqual(self.errors(example(True)), [])
        self.assertRejects(example(False), r"says 'phys_fn_000005' is not in the collision "
                                           r"pipeline, and it is")

    def test_rejects_a_row_two_slots_rule_differently(self):
        self.assertRejects(lambda i, r, p, s: p.__setitem__(0x3004, 0x1000),
                           r"row at 0x00001000 is named by slots T 0 object_model, T 1 collision")

    def test_rejects_a_slot_the_oracle_does_not_hold(self):
        self.assertRejects(lambda i, r, p, s: p.pop(0x3004),
                           r"table T slot 1 at 0x00003004 is not a relocated pointer")

    def test_rejects_a_helper_with_an_unrecorded_caller(self):
        def mutate(i, r, p, s):
            s[0x1038] = ruling_instruction(0x1038, "call", "0x10001040", target=0x1040)
        self.assertRejects(mutate, r"'phys_fn_000006' is a helper whose recorded call sites "
                                   r"\['0x00001004'\] are not its direct callers")

    def test_rejects_a_helper_an_unruled_row_calls(self):
        def mutate(i, r, p, s):
            s.pop(0x1004)
            s[0x1038] = ruling_instruction(0x1038, "call", "0x10001040", target=0x1040)
            r["members"][1]["callers"] = [{"site": "0x00001038", "row": "phys_fn_000005"}]
        self.assertRejects(mutate, r"is called from 'phys_fn_000005', which the ruling does not place")

    def test_rejects_a_helper_that_names_no_caller(self):
        # X13 and X14: an empty caller list passed the "every direct caller" check.
        def mutate(i, r, p, s):
            s.pop(0x1004)
            r["members"][1]["callers"] = []
        self.assertRejects(mutate, r"'phys_fn_000006' is a helper that records no caller")

    def test_rejects_a_member_at_a_slot_target(self):
        # X13: a member line re-ruling a collision slot's target outvoted its slot.
        def mutate(i, r, p, s):
            r["members"].append({"id": "phys_fn_000003", "rva": "0x00001010", "basis": "helper",
                                 "ruling": "object_model", "callers": []})
            self.row(i, "phys_fn_000003")["phase"] = 5
        self.assertRejects(mutate, r"member 'phys_fn_000003' is the target of T slot 1")

    def test_rejects_a_helper_whose_address_is_taken(self):
        self.assertRejects(lambda i, r, p, s: p.__setitem__(0x3100, 0x1040),
                           r"'phys_fn_000006' has its address taken at 0x00003100")

    def test_rejects_a_helper_inside_a_translation_unit_span(self):
        # X14 and X6b: a row a span places is not a helper.
        self.assertRejects(None, r"'phys_fn_000006' is inside Unit.cpp's span",
                           spans=[(0x1040, 0x1048, "Unit.cpp")])

    def test_rejects_a_helper_the_plan_pins_as_an_export(self):
        # X6: phys_fn_001704 is NxRayPlaneIntersect, pinned to Phase 3 by the plan.
        self.assertRejects(
            lambda i, r, p, s: i["exports"].append({"name": "NxRayPlaneIntersect",
                                                    "function_id": "phys_fn_000006"}),
            r"'phys_fn_000006' is an export the plan pins")

    def test_rejects_a_helper_that_stores_a_shape_table(self):
        self.assertRejects(
            lambda i, r, p, s: s.__setitem__(0x1044, ruling_instruction(
                0x1044, "mov", "dword ptr [esi], 0x10003000")),
            r"'phys_fn_000006' stores a shape table at 0x00001044")

    def test_rejects_a_constructor_that_stores_no_shape_table(self):
        self.assertRejects(
            lambda i, r, p, s: s[0x1024].__setitem__("operands", "dword ptr [esi], 0x10004000"),
            r"'phys_fn_000004' is a constructor because 0x00001024 stores a shape table")

    def test_rejects_a_collision_exception_no_collision_row_calls(self):
        def mutate(i, r, p, s):
            r["exceptions"] = [{"id": "phys_fn_000001", "rva": "0x00001000",
                                "disposition": "collision", "note": "x"}]
        self.assertRejects(mutate, r"exception 'phys_fn_000001' is ruled collision, and its direct "
                                   r"callers none are not all ruled rows of that phase")

    def test_rejects_a_phase_of_that_is_not_the_decision(self):
        self.assertRejects(
            lambda i, r, p, s: r.__setitem__("phase_of", {"collision": 5, "object_model": 3}),
            r"is not the user's decision")

    def test_rejects_a_shared_exception_off_phase_2(self):
        def mutate(i, r, p, s):
            r["exceptions"] = [{"id": "phys_fn_000003", "rva": "0x00001010",
                                "disposition": "shared", "phase": 4}]
        self.assertRejects(mutate, r"'phys_fn_000003' is shared runtime, which is Phase 2")


class TranslationUnitSpanTests(unittest.TestCase):
    """A translation-unit claim is the span its entry sits in."""

    SPANS = [(0x1000, 0x1020, "One.cpp"), (0x1100, 0x1120, "Two.cpp"),
             (0x1200, 0x1220, "Three.cpp")]
    PHASES = {"One.cpp": 3, "Two.cpp": 3, "Three.cpp": 7}

    def check(self, *rows, gaps=frozenset()):
        return validate_inventory.validate_translation_unit_spans(list(rows), self.SPANS,
                                                                  self.PHASES, gaps)

    def test_accepts_claims_the_spans_make(self):
        self.assertEqual(self.check(ruling_row("phys_fn_000001", 0x1010, 8, 3, "translation_unit"),
                                    ruling_row("phys_fn_000002", 0x1080, 8, 3, "enclosed_by_one_phase"),
                                    ruling_row("phys_fn_000003", 0x1018, 8, 5, "slot_ruling")), [])

    def test_rejects_a_row_moved_off_its_units_phase(self):
        # The shape of the review's first experiment: a row inside a Phase 3 unit
        # moved to Phase 5 with its provenance left as it was.
        errors = self.check(ruling_row("phys_fn_000001", 0x1010, 8, 5, "translation_unit"))
        self.assertIn("inside One.cpp, a phase 3 unit", errors[0])

    def test_rejects_a_translation_unit_claim_no_span_encloses(self):
        errors = self.check(ruling_row("phys_fn_000001", 0x1300, 8, 3, "translation_unit"))
        self.assertIn("inside no __FILE__ span", errors[0])

    def test_rejects_a_row_inside_a_span_placed_by_a_weaker_rule(self):
        errors = self.check(ruling_row("phys_fn_000001", 0x1010, 8, 5, "layout_adjacency"))
        self.assertIn("sits inside One.cpp, a phase 3 unit, but is placed by 'layout_adjacency'",
                      errors[0])

    def test_rejects_an_enclosure_two_phases_bracket(self):
        errors = self.check(ruling_row("phys_fn_000001", 0x1180, 8, 3, "enclosed_by_one_phase"))
        self.assertIn("between Two.cpp and Three.cpp", errors[0])

    def test_a_continuation_is_judged_by_its_entry(self):
        self.assertEqual(self.check(ruling_row("phys_fn_000001", 0x1030, 8, 3, "translation_unit",
                                               "continuation of the entry at 0x00001010")), [])

    def test_a_third_party_row_names_its_unit_elsewhere(self):
        row = dict(ruling_row("phys_fn_000001", 0x1300, 8, 4, "translation_unit"), third_party="opcode")
        self.assertEqual(self.check(row), [])

    def test_a_recorded_gap_is_accepted_and_must_stay_exact(self):
        row = ruling_row("phys_fn_000001", 0x1300, 8, 4, "translation_unit")
        self.assertEqual(self.check(row, gaps={"phys_fn_000001"}), [])
        errors = self.check(ruling_row("phys_fn_000001", 0x1010, 8, 3, "translation_unit"),
                            gaps={"phys_fn_000001"})
        self.assertIn("strike its pin from phase_pins.json", errors[0])


class OrphanedDeferralTests(unittest.TestCase):
    """A deferral whose driving phases have all passed is carried by an escalation's rows."""

    PROGRAM = {"phases": [{"phase": 2, "status": "pass"}, {"phase": 3, "status": "pass"},
                          {"phase": 5, "status": "pending"}, {"phase": 8, "status": "pending"}]}

    def check(self, driving, rows=None, inherited_by=(8,), deferred=True):
        ledgers = {2: {"deferred": [{"id": "phys_fn_000001", "driving_phases": list(driving)}]
                       if deferred else []}}
        escalation = {"id": "carried", "inherited_by": list(inherited_by)}
        if rows is not None:
            escalation["rows"] = rows
        return validate_inventory.validate_orphaned_deferrals(
            ledgers, {3: {"escalations": [escalation]}}, self.PROGRAM)

    def test_fires_on_a_deferral_every_driving_phase_of_which_has_passed(self):
        errors = self.check([3])
        self.assertEqual(len(errors), 1)
        self.assertIn("'phys_fn_000001' on driving phases 3, every one of which has passed", errors[0])

    def test_accepts_the_deferral_once_an_escalation_carries_it(self):
        self.assertEqual(self.check([3], rows=["phys_fn_000001"]), [])

    def test_an_escalation_only_passed_phases_inherit_carries_nothing(self):
        self.assertEqual(len(self.check([3], rows=["phys_fn_000001"], inherited_by=(3,))), 1)

    def test_a_deferral_naming_a_phase_still_to_run_is_not_orphaned(self):
        self.assertEqual(self.check([3, 5]), [])

    def test_a_deferral_naming_no_phase_is_not_orphaned(self):
        self.assertEqual(self.check([]), [])

    def test_rejects_carrying_a_row_no_ledger_defers(self):
        errors = self.check([3], rows=["phys_fn_000001"], deferred=False)
        self.assertIn("carries 'phys_fn_000001', which no ledger defers", errors[0])

    def test_rejects_escalation_rows_that_are_not_stable_ids(self):
        record = PhaseRecordTests().record()
        record["escalations"][0]["rows"] = ["phys_fn_1"]
        errors = validate_inventory.validate_phase_record(
            record, 2, PhaseRecordTests().program_phase(), PhaseRecordTests().totals(),
            list(PhaseRecordTests.EXCLUSIONS))
        self.assertTrue(any("which are not function stable IDs" in error for error in errors), errors)


class CommittedOrphanTests(unittest.TestCase):
    """The committed ledgers against the orphan rule: every one of the 38 is carried."""

    ROOT = TOOLS_DIR.parent

    def read(self):
        ledgers = {n: json.loads((self.ROOT / "gates" / f"phase{n}-closure.json").read_text(encoding="utf-8"))
                   for n in range(2, 8)}
        records = {n: json.loads((self.ROOT / "gates" / f"phase{n}.json").read_text(encoding="utf-8"))
                   for n in (2, 3, 4)}
        program = json.loads((self.ROOT / "program.json").read_text(encoding="utf-8"))
        return ledgers, records, program

    def carried(self, records, name):
        return next(e for e in records[4]["escalations"] if e["id"] == name)

    def test_every_orphan_is_carried(self):
        ledgers, records, program = self.read()
        self.assertEqual(validate_inventory.validate_orphaned_deferrals(ledgers, records, program), [])
        self.assertEqual([len(self.carried(records, name)["rows"]) for name in (
            "mesh_column_blocks_phase3_mesh_deferrals", "deferrals_left_on_passed_phase_4",
            "deferrals_left_on_passed_phases_3_and_4")], [13, 7, 18])

    def test_the_rule_fires_when_a_carried_row_is_dropped(self):
        ledgers, records, program = self.read()
        self.carried(records, "mesh_column_blocks_phase3_mesh_deferrals")["rows"].remove("phys_fn_001757")
        errors = validate_inventory.validate_orphaned_deferrals(ledgers, records, program)
        self.assertEqual(len(errors), 1)
        self.assertIn("'phys_fn_001757'", errors[0])

    def test_the_rule_fires_on_all_eighteen_without_their_escalation(self):
        ledgers, records, program = self.read()
        records[4]["escalations"].remove(self.carried(records, "deferrals_left_on_passed_phases_3_and_4"))
        self.assertEqual(len(validate_inventory.validate_orphaned_deferrals(ledgers, records, program)), 18)


class CommittedShapeRulingTests(unittest.TestCase):
    """The committed census against its ruling and its spans, and the review's experiments.

    Each wrong move a review showed the validator accepting is replayed here on a
    copy of the committed census or ruling, so the check that rejects it cannot be
    loosened without a test saying so.
    """

    ROOT = TOOLS_DIR.parent

    @classmethod
    def setUpClass(cls):
        def read(*parts):
            return json.loads(cls.ROOT.joinpath(*parts).read_text(encoding="utf-8"))
        cls.inventory = read("inventory.json")
        cls.ruling = read(validate_inventory.SHAPE_RULING_NAME)
        pe = read("oracle", "pe.json")
        corpus = read("oracle", "capstone", "manifest.json")
        ghidra = read("oracle", "ghidra", "manifest.json")
        cls.pointers = {slot["rva"]: slot["target_rva"] for slot in pe["pointers"]
                        if slot["relocated"] and slot["target_rva"] is not None}
        cls.instructions = {int(entry["rva"], 16): entry for entry in corpus["instructions"]}
        cls.tail_calls = corpus["tail_calls"]
        cls.image_base = pe["image"]["image_base"]
        cls.imports = {entry["iat_rva"] for entry in pe["imports"]}
        cls.spans, cls.phases = validate_inventory.translation_unit_spans_of(
            cls.inventory["functions"], ghidra["strings"], ghidra["references"])
        pins, _ = validate_inventory.read_phase_pins(read(validate_inventory.PHASE_PINS_NAME))
        cls.without_span = frozenset(identifier for identifier, pin in pins.items()
                                     if pin["phase_provenance"] == "translation_unit")

    def errors(self, mutate=None, rule=None):
        inventory, ruling = copy.deepcopy(self.inventory), copy.deepcopy(self.ruling)
        rows = {row["id"]: row for row in inventory["functions"]}
        if mutate:
            mutate(rows)
        if rule:
            rule(ruling)
        return (validate_inventory.validate_shape_ruling(
                    inventory, ruling, self.pointers, self.instructions, self.tail_calls,
                    self.image_base, self.imports, self.spans)
                + validate_inventory.validate_translation_unit_spans(
                    inventory["functions"], self.spans, self.phases, self.without_span))

    def assertRejects(self, pattern, mutate=None, rule=None):
        errors = self.errors(mutate, rule)
        self.assertTrue(any(re.search(pattern, error) for error in errors), errors)

    @staticmethod
    def table(ruling, cls):
        return next(table for table in ruling["tables"] if table["class"] == cls)

    def test_the_committed_census_agrees_with_its_ruling_and_spans(self):
        self.assertEqual(self.errors(), [])

    def test_the_pipeline_derives_every_committed_verdict(self):
        _, _, dispatched, interfaces, errors = validate_inventory.collision_pipeline(
            self.ruling, self.inventory["functions"], self.instructions, self.tail_calls,
            self.pointers, self.imports, self.image_base)
        self.assertEqual(errors, [])
        self.assertEqual(validate_inventory.slot_verdict_errors(self.ruling, dispatched), [])
        self.assertEqual({cls for cls, name in interfaces.items() if name != "BASE"}, {"BOX_HULL"})

    def test_rejects_the_box_hull_slots_that_were_ruled_object_model(self):
        # Critical 1: phys_fn_000959, BOX_HULL slot 10, calls slots 6, 7 and 8 on its
        # own this at 0x0002131c, 0x00021323 and 0x0002132c.
        def rule(ruling):
            for index in (6, 7, 8):
                self.table(ruling, "BOX_HULL")["slots"][index] = "object_model"
        for site in ("0x0002131c", "0x00021323", "0x0002132c"):
            self.assertRejects(rf"object_model, but the collision pipeline dispatches it at {site}",
                               rule=rule)

    def test_rejects_x1_a_coordinated_flip_of_a_box_hull_collision_slot(self):
        def rule(ruling):
            self.table(ruling, "BOX_HULL")["slots"][3] = "object_model"
        self.assertRejects(r"rules BOX_HULL slot 3 object_model, but the collision pipeline "
                           r"dispatches it", lambda rows: rows["phys_fn_000961"].update(phase=5),
                           rule)

    def test_rejects_x2_a_slot_flipped_on_a_call_that_is_not_the_pipelines(self):
        # X2 cited an allocator free in phys_fn_001461 as BOX slot 3. The receiver there
        # is what phys_fn_004803 returns, typed once for all its call sites.
        def rule(ruling):
            self.table(ruling, "BOX")["slots"][3] = "collision"
        self.assertRejects(r"rules BOX slot 3 collision, but the collision pipeline never "
                           r"dispatches it", lambda rows: rows["phys_fn_000945"].update(phase=3),
                           rule)

        def retyped(ruling):
            rule(ruling)
            ruling["receivers"].append({"site": "0x0002ae7a", "row": "phys_fn_001461",
                                        "receiver": ["BOX"], "note": "fabricated"})
        self.assertRejects(r"types 0x0002ae7a twice", rule=retyped)

    def test_rejects_x13_an_empty_helper_re_ruling_a_collision_slot(self):
        def rule(ruling):
            ruling["members"].append({"id": "phys_fn_000949", "rva": "0x00020880",
                                      "basis": "helper", "ruling": "object_model", "callers": []})
        self.assertRejects(r"member 'phys_fn_000949' is the target of BOX slot 5",
                           lambda rows: rows["phys_fn_000949"].update(phase=5), rule)

    def test_rejects_x14_a_phase_6_unit_row_claimed_as_a_helper(self):
        def rule(ruling):
            ruling["members"].append({"id": "phys_fn_003940", "rva": "0x0008eef0",
                                      "basis": "helper", "ruling": "object_model", "callers": []})
        self.assertRejects(r"'phys_fn_003940' is a helper that records no caller",
                           lambda rows: rows["phys_fn_003940"].update(
                               phase=5, phase_provenance="slot_ruling"), rule)
        self.assertRejects(r"'phys_fn_003940' is inside .* span",
                           lambda rows: rows["phys_fn_003940"].update(
                               phase=5, phase_provenance="slot_ruling"), rule)

    def test_rejects_x6_an_export_pin_claimed_as_a_helper(self):
        def rule(ruling):
            ruling["members"].append({"id": "phys_fn_001704", "rva": "0x00036bb0",
                                      "basis": "helper", "ruling": "collision",
                                      "callers": [{"site": "0x0002539d", "row": "phys_fn_001261"},
                                                  {"site": "0x00029724", "row": "phys_fn_001407"}]})
        self.assertRejects(r"'phys_fn_001704' is an export the plan pins",
                           lambda rows: rows["phys_fn_001704"].update(phase_provenance="slot_ruling"),
                           rule)

    def test_rejects_x6b_a_phase_4_unit_row_claimed_as_a_helper(self):
        caller = sorted(address for address, found in self.instructions.items()
                        if found.get("target_rva") == "0x" + self.inventory_rva("phys_fn_002241")
                        and not found["indirect"])[0]

        def rule(ruling):
            ruling["members"].append({"id": "phys_fn_002241",
                                      "rva": "0x" + self.inventory_rva("phys_fn_002241"),
                                      "basis": "helper", "ruling": "object_model",
                                      "callers": [{"site": f"0x{caller:08x}",
                                                   "row": "phys_fn_001397"}]})
        self.assertRejects(r"'phys_fn_002241' is inside .* span",
                           lambda rows: rows["phys_fn_002241"].update(
                               phase=5, phase_provenance="slot_ruling"), rule)

    def inventory_rva(self, identifier):
        return next(row["rva"] for row in self.inventory["functions"]
                    if row["id"] == identifier)[2:]

    def test_the_worked_examples_hold(self):
        examples = {(example["class"], example["slot"]): example
                    for example in self.ruling["pipeline"]["worked_examples"]}
        self.assertEqual(sorted(examples), [("BOX", 9), ("COMPOUND", 6)])
        self.assertEqual(examples[("COMPOUND", 6)]["path"][4],
                         {"row": "phys_fn_000022", "site": "0x00001855", "in_pipeline": False})

        def rule(ruling):
            ruling["pipeline"]["worked_examples"][0]["path"][4]["in_pipeline"] = True
        self.assertRejects(r"says 'phys_fn_000022' is in the collision pipeline, and it is not",
                           rule=rule)

    def test_rejects_moving_a_contact_row_out_of_its_phase_3_unit(self):
        # phys_fn_001847 sits inside ContactConvexHeightfield.cpp.
        self.assertRejects(r"'phys_fn_001847'.*ContactConvexHeightfield.cpp",
                           lambda rows: rows["phys_fn_001847"].__setitem__("phase", 5))

    def test_rejects_moving_shapebase_back_to_phase_3(self):
        self.assertRejects(r"'phys_fn_001273' is placed by the shape ruling at phase 5",
                           lambda rows: rows["phys_fn_001273"].__setitem__("phase", 3))

    def test_rejects_relabelling_shapebase_to_layout_adjacency(self):
        self.assertRejects(r"'phys_fn_001273' is placed by the shape ruling",
                           lambda rows: rows["phys_fn_001273"].__setitem__(
                               "phase_provenance", "layout_adjacency"))

    def test_rejects_moving_a_closed_slot_5_row_to_phase_5(self):
        # phys_fn_001377 is SPHERE slot 5, a collision slot, closed by Phase 3.
        self.assertRejects(r"'phys_fn_001377' is placed by the shape ruling at phase 3",
                           lambda rows: rows["phys_fn_001377"].__setitem__("phase", 5))

    def test_the_three_annotated_continuations_resolve_to_their_entries(self):
        rows = {row["id"]: row for row in self.inventory["functions"]}
        self.assertEqual({identifier: validate_inventory._entry_rva(rows[identifier])
                          for identifier in ("phys_fn_005590", "phys_fn_005610", "phys_fn_005646")},
                         {"phys_fn_005590": 0xF25B0, "phys_fn_005610": 0xF2EF0,
                          "phys_fn_005646": 0xF3DC0})


class CommittedRegenerationTests(unittest.TestCase):
    """The committed phase column is the generator's output, except exactly the pins."""

    ROOT = TOOLS_DIR.parent

    @classmethod
    def setUpClass(cls):
        import reconcile_analysis

        def read(*parts):
            return json.loads(cls.ROOT.joinpath(*parts).read_text(encoding="utf-8"))
        cls.inventory = read("inventory.json")
        cls.pins, errors = validate_inventory.read_phase_pins(read(validate_inventory.PHASE_PINS_NAME))
        assert not errors, errors
        correspondence, _ = validate_inventory.read_source_correspondence(cls.ROOT)
        result = reconcile_analysis.reconcile(
            read("oracle", "pe.json"), read("oracle", "ghidra", "manifest.json"),
            read("oracle", "capstone", "manifest.json"), cls.inventory,
            read(validate_inventory.SHAPE_RULING_NAME),
            reconcile_analysis.third_party_rvas(correspondence), cls.pins)
        cls.regenerated, cls.unpinned = result["inventory"], result["unpinned"]

    def errors(self, mutate=None, pins=None):
        inventory = copy.deepcopy(self.inventory)
        rows = {row["id"]: row for kind in ("functions", "data_objects") for row in inventory[kind]}
        if mutate:
            mutate(rows)
        return validate_inventory.validate_regeneration(inventory, self.regenerated,
                                                        pins or self.pins, self.unpinned)

    def test_the_committed_census_is_the_generators_output(self):
        self.assertEqual(self.errors(), [])
        self.assertEqual(len(self.pins), 72)
        self.assertEqual(sum(pin["phase_provenance"] == "translation_unit"
                             for pin in self.pins.values()), 52)

    def test_fires_on_the_six_rows_the_committed_census_held_against_the_generator(self):
        before = {"phys_fn_000887": (3, "layout_adjacency"), "phys_fn_000889": (3, "layout_adjacency"),
                  "phys_fn_000903": (3, "layout_adjacency"), "phys_fn_002352": (5, "layout_adjacency"),
                  "phys_fn_002354": (2, "shared_by_callers"), "phys_data_005007": (3, "reading_sites")}

        def mutate(rows):
            for identifier, (phase, provenance) in before.items():
                rows[identifier].update(phase=phase, phase_provenance=provenance)
        errors = self.errors(mutate)
        self.assertEqual(sorted(error.split()[0] for error in errors), sorted(before))

    def test_fires_on_a_knock_on_row_moved_against_its_callers(self):
        # phys_fn_000915's callers are Phase 7 and shared runtime; moving the row it
        # places, phys_fn_000903, back to Phase 3 is the shape of the committed error.
        errors = self.errors(lambda rows: rows["phys_fn_000903"].update(phase=3))
        self.assertEqual(len(errors), 1)
        self.assertIn("phys_fn_000903 is phase 3 'callers' in the census, and the generator places "
                      "it at phase 7 'callers'", errors[0])

    def test_fires_on_x3_a_pinned_row_moved_off_its_pin(self):
        # X3: phys_fn_005440, a translation_unit row with no span, moved 4 -> 3.
        errors = self.errors(lambda rows: rows["phys_fn_005440"].update(phase=3))
        self.assertIn("phys_fn_005440 is phase 3 'translation_unit' in the census, and the "
                      "generator places it at phase 4 'translation_unit', which is its pin", errors[0])

    def test_strikes_a_pin_the_generator_already_agrees_with(self):
        pins = dict(self.pins, phys_fn_000887={
            "id": "phys_fn_000887", "rva": "0x0001e910", "phase": 7, "phase_provenance": "callers",
            "reason": "x", "decided_by": "0000000"})
        unpinned = dict(self.unpinned, phys_fn_000887=(7, "callers"))
        errors = validate_inventory.validate_regeneration(self.inventory, self.regenerated, pins,
                                                          unpinned)
        self.assertEqual(errors, ["phase_pins.json pins phys_fn_000887 to what the generator already "
                                  "gives it; strike the pin"])


class PhasePinTests(unittest.TestCase):
    """phase_pins.json's entries are checked for what each one must say."""

    PIN = {"id": "phys_fn_000001", "rva": "0x00001000", "phase": 4,
           "phase_provenance": "translation_unit", "reason": "why", "decided_by": "cc37f94e"}

    def read(self, **change):
        return validate_inventory.read_phase_pins({"pins": [dict(self.PIN, **change)]})

    def test_reads_a_pin(self):
        self.assertEqual(self.read(), ({"phys_fn_000001": self.PIN}, []))

    def test_rejects_a_pin_with_no_reason_or_commit(self):
        self.assertIn("with no reason", self.read(reason=" ")[1][0])
        self.assertIn("without naming the commit", self.read(decided_by="P4 Task 1c")[1][0])

    def test_rejects_a_pin_the_census_cannot_record(self):
        self.assertIn("which the census cannot record", self.read(phase_provenance="vibes")[1][0])

    def test_rejects_a_pin_on_a_data_object(self):
        self.assertIn("not a function stable ID", self.read(id="phys_data_000001")[1][0])


if __name__ == "__main__":
    unittest.main()
