import json
import hashlib
import gzip
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
REPO_DIR = Path(__file__).resolve().parents[5]
AUDITOR = TOOLS_DIR / "verify_static_proofs.py"
sys.path.insert(0, str(TOOLS_DIR))

from verify_static_proofs import audit_inventory  # noqa: E402


class StaticProofAuditTests(unittest.TestCase):
    def test_legacy_evidence_manifests_pin_uncompressed_and_normalized_text_hashes(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            evidence_root = root / "evidence"
            packet = evidence_root / "raw" / "fixture"
            packet.mkdir(parents=True)
            source = b"first line\nsecond line\n"
            compressed = gzip.compress(source, mtime=0)
            (packet / "gate.log.gz").write_bytes(compressed)
            (packet / "direct-hash.log.gz").write_bytes(compressed)
            (packet / "mutation.patch.txt").write_bytes(
                b"first line\r\nsecond line\r\n")
            (packet / "manifest.json").write_text(json.dumps({
                "files": [
                    {"file": "gate.log.gz", "sha256": hashlib.sha256(source).hexdigest(),
                     "source_bytes": len(source), "compressed_bytes": len(compressed)},
                    {"file": "mutation.patch.txt",
                     "sha256": hashlib.sha256(source).hexdigest()},
                ]
            }), encoding="utf-8")
            packet_dict = evidence_root / "raw" / "fixture-dict"
            packet_dict.mkdir()
            (packet_dict / "direct-hash.log.gz").write_bytes(compressed)
            (packet_dict / "manifest.json").write_text(json.dumps({
                "files": {"direct-hash.log.gz": {
                    "sha256": hashlib.sha256(compressed).hexdigest(),
                    "compressed_bytes": len(compressed),
                    "uncompressed_sha256": hashlib.sha256(source).hexdigest(),
                }}
            }), encoding="utf-8")

            from verify_static_proofs import _verify_evidence_manifests

            findings = _verify_evidence_manifests(evidence_root)

            self.assertFalse(findings, findings)

    def test_analysis_manifests_are_loaded_once_for_repeated_row_references(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source_root = root / "src"
            oracle_root = root / "oracle"
            evidence_root = root / "evidence"
            (oracle_root / "ghidra").mkdir(parents=True)
            (oracle_root / "capstone").mkdir(parents=True)
            source_root.mkdir()
            evidence_root.mkdir()
            (source_root / "sample.cpp").write_text(
                "void sample() {}\n", encoding="utf-8")
            (oracle_root / "ghidra" / "manifest.json").write_text(
                json.dumps({"functions": [{"rva": "0x1000"}]}), encoding="utf-8")
            (oracle_root / "capstone" / "manifest.json").write_text(
                json.dumps({"entries": [{"rva": "0x1000"}]}), encoding="utf-8")
            inventory = {"functions": [
                {
                    "id": f"phys_fn_{index:06d}", "rva": "0x00001000",
                    "size": 8, "kind": "code", "state": "closed",
                    "implementation": "sample.cpp", "implementation_symbol": "sample",
                    "ghidra_ref": "oracle/ghidra/manifest.json#/functions/0",
                    "capstone_ref": "oracle/capstone/manifest.json#/entries/0",
                    "static_proof": "x86 row has a reviewed source mapping.",
                }
                for index in range(1, 4)
            ], "data_objects": []}

            report, _ = audit_inventory(
                inventory, source_root, root, evidence_root,
                {"function_ids": [row["id"] for row in inventory["functions"]],
                 "slots": []}, {"uses": []})

            self.assertEqual(report["analysis_inputs"]["reference_manifests_loaded"], 2)
            self.assertFalse(report["findings"], report["findings"])

    def test_compressed_evidence_manifests_use_the_compressed_digest(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            evidence_root = root / "evidence"
            packet = evidence_root / "raw" / "fixture"
            packet.mkdir(parents=True)
            payload = b"compressed-log-placeholder"
            (packet / "gate.log.gz").write_bytes(payload)
            digest = hashlib.sha256(payload).hexdigest()
            (packet / "manifest.json").write_text(json.dumps({
                "files": [{
                    "path": "gate.log.gz",
                    "sha256_gzip": digest,
                    "sha256_uncompressed": "f" * 64,
                }]
            }), encoding="utf-8")

            from verify_static_proofs import _verify_evidence_manifests

            findings = _verify_evidence_manifests(evidence_root)

            self.assertFalse(findings, findings)

    def test_nonterminal_code_row_is_reported_as_audit_debt(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source_root = root / "src"
            source_root.mkdir()
            (source_root / "sample.cpp").write_text(
                "void sample() {}\n", encoding="utf-8")
            inventory = {
                "functions": [{
                    "id": "phys_fn_000001",
                    "rva": "0x00001000",
                    "size": 8,
                    "kind": "code",
                    "state": "reconstructed",
                    "implementation": "sample.cpp",
                    "implementation_symbol": "sample",
                    "ghidra_ref": None,
                    "capstone_ref": None,
                }],
                "data_objects": [],
            }
            inventory_path = root / "inventory.json"
            report_path = root / "static-proof.json"
            inventory_path.write_text(json.dumps(inventory), encoding="utf-8")

            result = subprocess.run(
                [sys.executable, str(AUDITOR), "--inventory", str(inventory_path),
                 "--source-root", str(source_root), "--output", str(report_path)],
                check=False, capture_output=True, text=True)

            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertTrue(report_path.is_file(), result.stdout + result.stderr)
            report = json.loads(report_path.read_text(encoding="utf-8"))
            self.assertTrue(any(
                finding["code"] == "code_row_not_closed"
                and finding["id"] == "phys_fn_000001"
                for finding in report["findings"]))

    def test_ghidra_generic_data_label_is_not_a_structural_proof(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            evidence_root = root / "evidence"
            evidence_root.mkdir()
            report, _ = audit_inventory({
                "functions": [],
                "data_objects": [{
                    "id": "phys_data_000001", "type": "ghidra_data",
                    "state": "classified", "owner": None,
                    "structural_proof": "Ghidra typed these bytes as data",
                }],
            }, root / "src", root, evidence_root, {"function_ids": [], "slots": []},
                {"uses": []})

            self.assertIn(
                ("data_structure_unproven", "phys_data_000001"),
                {(finding["code"], finding["id"]) for finding in report["findings"]})

    def test_audit_reports_source_reference_hash_dispatch_field_data_and_artifact_gaps(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source_root = root / "src"
            evidence_root = root / "evidence"
            source_root.mkdir()
            packet = evidence_root / "raw" / "fixture"
            packet.mkdir(parents=True)
            (packet / "baseline.bin").write_bytes(b"verified bytes")
            (packet / "manifest.json").write_text(json.dumps({
                "files": {"baseline.bin": "0" * 64}
            }), encoding="utf-8")
            inventory = {
                "functions": [
                    {
                        "id": "phys_fn_000001",
                        "rva": "0x00001000",
                        "size": 8,
                        "kind": "code",
                        "state": "reconstructed",
                        "implementation": None,
                        "implementation_symbol": None,
                        "ghidra_ref": None,
                        "capstone_ref": None,
                    },
                    {
                        "id": "phys_fn_000002",
                        "rva": "0x00001008",
                        "size": 4,
                        "kind": "compiler_artifact",
                        "state": "classified",
                        "implementation": None,
                        "implementation_symbol": None,
                        "ghidra_ref": None,
                        "capstone_ref": None,
                        "static_proof": None,
                    },
                ],
                "data_objects": [{
                    "id": "phys_data_000001",
                    "type": "dispatch_table",
                    "size": 8,
                    "owner": None,
                    "state": "classified",
                    "structural_proof": "two recovered function pointers",
                }],
            }
            inventory_path = root / "inventory.json"
            dispatch_path = root / "dispatch.json"
            fields_path = root / "fields.json"
            report_path = root / "static-proof.json"
            inventory_path.write_text(json.dumps(inventory), encoding="utf-8")
            dispatch_path.write_text(json.dumps({"slots": [{
                "id": "NxFixture::slot[7]", "function_id": "phys_fn_999999"
            }]}), encoding="utf-8")
            fields_path.write_text(json.dumps({"uses": [{
                "id": "NxFixture+0x14", "function_id": "phys_fn_000001",
                "explained": False,
            }]}), encoding="utf-8")

            result = subprocess.run(
                [sys.executable, str(AUDITOR), "--inventory", str(inventory_path),
                 "--source-root", str(source_root), "--output", str(report_path),
                 "--repo-root", str(root), "--evidence-root", str(evidence_root),
                 "--dispatch-map", str(dispatch_path), "--field-map", str(fields_path)],
                check=False, capture_output=True, text=True)

            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertTrue(report_path.is_file(), result.stdout + result.stderr)
            report = json.loads(report_path.read_text(encoding="utf-8"))
            findings = {(finding["code"], finding["id"])
                        for finding in report["findings"]}
            self.assertTrue({
                ("source_mapping_missing", "phys_fn_000001"),
                ("ghidra_reference_missing", "phys_fn_000001"),
                ("capstone_reference_missing", "phys_fn_000001"),
                ("unknown_vtable_slot", "NxFixture::slot[7]"),
                ("unexplained_field_use", "NxFixture+0x14"),
                ("significant_data_unowned", "phys_data_000001"),
                ("artifact_classification_missing", "phys_fn_000002"),
            } <= findings, report["findings"])
            self.assertTrue(any(
                code == "evidence_hash_mismatch" and identity.endswith("baseline.bin")
                for code, identity in findings), report["findings"])


if __name__ == "__main__":
    unittest.main()
