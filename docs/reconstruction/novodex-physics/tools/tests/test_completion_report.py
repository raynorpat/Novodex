import json
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
REPO_DIR = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(TOOLS_DIR))

from report_completion import build_report  # noqa: E402


class CompletionReportTests(unittest.TestCase):
    def test_backlog_counts_code_bytes_and_artifacts_separately(self):
        inventory = {
            "functions": [
                {"id": "phys_fn_000001", "rva": "0x1000", "size": 6,
                 "kind": "code", "phase": 2, "state": "reconstructed",
                 "source": "src/a.cpp", "implementation": "src/a.cpp",
                 "implementation_symbol": "A::run", "static_proof": "listed",
                 "dynamic_proof": "", "notes": ""},
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
