import json
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
REPO_DIR = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(TOOLS_DIR))

from report_completion import _sha256, build_report, write_report  # noqa: E402


class CompletionReportTests(unittest.TestCase):
    def test_static_candidate_map_reference_is_reported_without_dynamic_proof(self):
        candidate_map = {
            "path": "build/Release/NxPhysics.map",
            "symbol": "_phys_fn_000002@16",
            "address": "0x1004e050",
            "object": "RecoveredRows.obj",
        }
        inventory = {
            "functions": [{
                "id": "phys_fn_000002", "rva": "0x1030", "size": 49,
                "kind": "code", "phase": 5, "state": "statically_reviewed",
                "source": "Physics/src/RecoveredRows.cpp",
                "implementation": "Physics/src/RecoveredRows.cpp",
                "implementation_symbol": "_phys_fn_000002@16",
                "candidate_map_reference": candidate_map,
                "static_proof": "NxPhysicsRangeIterationTests exercises the row.",
                "dynamic_proof": None,
            }],
            "exports": [],
        }

        report = build_report(inventory, [], {})

        implementation = report["rows"][0]["implementation"]
        self.assertTrue(implementation["candidate_map_reference_recorded"])
        self.assertEqual(implementation["candidate_map_reference_basis"], "inventory")
        self.assertEqual(implementation["candidate_map_reference"], candidate_map)

    def test_subobject_forwarder_has_candidate_source_and_map_symbol(self):
        report = build_report_from_repo(REPO_DIR)
        row = next(row for row in report["rows"] if row["id"] == "phys_fn_000004")

        self.assertEqual(row["implementation"]["path"], "Physics/src/ObjectModel.cpp")
        self.assertEqual(row["implementation"]["symbol"], "nxForwardSubobjectCall")
        self.assertEqual(row["implementation"]["candidate_map_reference"], {
            "path": "build/Release/NxPhysics.map",
            "symbol": "?nxForwardSubobjectCall@@YAIPAX0@Z",
            "address": "0x1003e5f0",
            "object": "ObjectModel.obj",
        })

    def test_generated_report_uses_lf_bytes_on_every_platform(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output = Path(temp_dir) / "report.json"
            write_report(output, {"rows": ["first", "second"]})

            self.assertEqual(
                output.read_bytes(),
                b'{\n  "rows": [\n    "first",\n    "second"\n  ]\n}\n')

    def test_sha256_normalizes_text_line_endings(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            lf_path = Path(temp_dir) / "lf.json"
            crlf_path = Path(temp_dir) / "crlf.json"
            lf_path.write_bytes(b'{\n  "phase": 5\n}\n')
            crlf_path.write_bytes(b'{\r\n  "phase": 5\r\n}\r\n')

            self.assertEqual(_sha256(lf_path), _sha256(crlf_path))

    def test_backlog_counts_code_bytes_and_artifacts_separately(self):
        inventory = {
            "functions": [
                {"id": "phys_fn_000001", "rva": "0x1000", "size": 6,
                 "kind": "code", "phase": 2, "state": "reconstructed",
                 "source": "src/a.cpp", "implementation": "src/a.cpp",
                 "implementation_symbol": "A::run", "static_proof": "listed",
                 "dynamic_proof": "candidate symbol retained in NxPhysics.map", "notes": ""},
                {"id": "phys_fn_000002", "rva": "0x2000", "size": 10,
                 "kind": "code", "phase": 2, "state": "discovered",
                 "source": None, "static_proof": None, "dynamic_proof": None,
                 "notes": ""},
                {"id": "phys_fn_000003", "rva": "0x3000", "size": 20,
                 "kind": "compiler_artifact", "phase": 2, "state": "classified",
                 "source": None, "static_proof": "compiler output", "dynamic_proof": "",
                 "notes": ""},
            ],
            "exports": [{"function_id": "phys_fn_000001", "name": "A_Run"}],
            "data_objects": [{"size": 3}, {"size": 4}],
        }
        closures = [{
            "phase": 2,
            "closed": [{"id": "phys_fn_000001", "proof": "differential_falsified",
                        "gate": "NxPhysicsUnitTests", "falsification": {"detected": "delta=1"}}],
            "deferred": [{"id": "phys_fn_000002", "reason": "not_reconstructed_in_phase",
                          "driving_phases": [], "note": "recover implementation"}],
        }]
        capstone = {"instructions": [
            {"rva": "0x1000", "size": 5, "flow": "call", "indirect": False,
             "target_rva": "0x2000"},
            {"rva": "0x1005", "size": 1, "flow": "call", "indirect": True,
             "target_rva": None},
        ]}

        report = build_report(inventory, closures, capstone)

        self.assertEqual(report["summary"]["code_rows"], 2)
        self.assertEqual(report["summary"]["code_bytes"], 16)
        self.assertEqual(report["summary"]["artifact_rows"], 1)
        self.assertEqual(report["summary"]["artifact_bytes"], 20)
        self.assertEqual(report["summary"]["data_objects"], 2)
        self.assertEqual(report["summary"]["terminally_closed_code_rows"], 0)
        self.assertEqual(report["summary"]["artifact_states"], {"classified": 1})
        self.assertEqual(report["summary"]["data_states"], {"unknown": 2})
        self.assertEqual(report["summary"]["intermediate_closure_ledgers"]["2"][
            "closed_rows"], 1)
        first, second = report["rows"]
        self.assertEqual(first["reachability"]["status"], "direct_export")
        self.assertTrue(first["implementation"]["candidate_map_reference_recorded"])
        self.assertEqual(first["implementation"]["candidate_map_reference_basis"],
                         "dynamic_proof")
        self.assertEqual(first["evidence"]["strength"], "row_specific_gate_falsification")
        self.assertEqual(first["dependencies"]["direct_rows"],
                         ["phys_fn_000002"])
        self.assertEqual(first["unresolved_dependencies"]["unclosed_direct_callees"],
                         ["phys_fn_000002"])
        self.assertEqual(first["dependencies"]["unresolved_indirect_calls"], 1)
        self.assertEqual(second["next_packet"]["action"],
                         "recover_contract_and_reconstruct")

    def test_cli_snapshot_keeps_the_three_opcode_rows_as_code(self):
        report = build_report_from_repo(REPO_DIR)
        by_id = {row["id"]: row for row in report["rows"]}
        expected_symbols = {
            "phys_fn_005493": "Segment::SquareDistance",
            "phys_fn_005517": "AABBTreeNode::_BuildHierarchy",
            "phys_fn_005523": "AABBTree::Build",
        }
        for row_id, expected_symbol in expected_symbols.items():
            with self.subTest(row_id=row_id):
                row = by_id[row_id]
                self.assertEqual(row["kind"], "code")
                self.assertEqual(row["phase"], 4)
                self.assertIsNotNone(row["implementation"]["path"])
                self.assertTrue(row["implementation"]["file_exists"])
                self.assertEqual(row["implementation"]["symbol"], expected_symbol)
                self.assertNotEqual(row["next_packet"]["action"], "classified_artifact_review")

        self.assertTrue(by_id["phys_fn_005493"]["implementation"][
            "candidate_map_reference_recorded"])

        target = REPO_DIR / "docs/reconstruction/novodex-physics/completion-backlog.json"
        snapshot = json.loads(target.read_text(encoding="utf-8"))
        gates = REPO_DIR / "docs/reconstruction/novodex-physics/gates"
        expected_hashes = {
            path.relative_to(REPO_DIR).as_posix(): _sha256(path)
            for path in sorted(gates.glob("phase*-closure.json"))
        }
        self.assertEqual(snapshot["source"]["closure_ledgers_sha256"], expected_hashes)


def build_report_from_repo(repo_root):
    inventory_path = repo_root / "docs/reconstruction/novodex-physics/inventory.json"
    gates_dir = repo_root / "docs/reconstruction/novodex-physics/gates"
    capstone_path = repo_root / "docs/reconstruction/novodex-physics/oracle/capstone/manifest.json"
    inventory = json.loads(inventory_path.read_text(encoding="utf-8"))
    closures = [json.loads(path.read_text(encoding="utf-8"))
                for path in sorted(gates_dir.glob("phase*-closure.json"))]
    capstone = json.loads(capstone_path.read_text(encoding="utf-8"))
    return build_report(inventory, closures, capstone, repo_root)


if __name__ == "__main__":
    unittest.main()
