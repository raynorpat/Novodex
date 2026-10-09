import hashlib
import json
import pathlib
import tempfile
import unittest

import sys

TOOLS = pathlib.Path(__file__).resolve().parents[1]
TESTS = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
sys.path.insert(0, str(TESTS))

import oracle_proof
import test_gate_targets


TARGET = "NxPhysicsJointSupportTests"
ORACLE_SHA = "a" * 64
SOURCE = "tests/PhysicsJointSupportTests.cpp"
SOURCE_BYTES = b"fixed joint support inputs"
INPUT_DIGEST = "85a7065a061c36cd"
OUTPUT_DIGEST = "88b713b7bc0870c9"


class OracleProofTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = pathlib.Path(self.temporary.name)
        source = self.root / SOURCE
        source.parent.mkdir(parents=True)
        source.write_bytes(SOURCE_BYTES)
        self.manifest = {
            "schema_version": 1,
            "targets": {
                TARGET: {
                    "oracle_sha256": ORACLE_SHA,
                    "fixture_source": SOURCE,
                    "fixture_source_sha256": hashlib.sha256(SOURCE_BYTES).hexdigest(),
                    "cases": 2,
                    "input_digest_fnv64": INPUT_DIGEST,
                    "oracle_output_digest_fnv64": OUTPUT_DIGEST,
                }
            },
        }
        self.transcript = [
            "joint_support kind5 cases=2 oracle=%s candidate=%s mismatches=0" %
            (OUTPUT_DIGEST, "1234567890abcdef"),
            "joint_support inputs=2 digest=%s" % INPUT_DIGEST,
        ]

    def verify(self, lines=None, oracle_sha=ORACLE_SHA):
        return oracle_proof.verify_oracle_transcript(
            self.manifest, TARGET, self.transcript if lines is None else lines,
            oracle_sha, self.root)

    def test_pins_oracle_inputs_cases_and_output_without_pinning_candidate_digest(self):
        failures, proof = self.verify()
        self.assertEqual(failures, [])
        self.assertEqual(proof["cases"], 2)
        self.assertEqual(proof["input_digest_fnv64"], INPUT_DIGEST)
        self.assertEqual(proof["oracle_output_digest_fnv64"], OUTPUT_DIGEST)

        changed_candidate = [
            self.transcript[0].replace("1234567890abcdef", "ffffffffffffffff")
                .replace("mismatches=0", "mismatches=1"),
            self.transcript[1],
        ]
        failures, _ = self.verify(lines=changed_candidate)
        self.assertEqual(failures, [])

    def test_rejects_changed_oracle_output_digest(self):
        lines = list(self.transcript)
        lines[0] = lines[0].replace(OUTPUT_DIGEST, "0000000000000000")
        failures, _ = self.verify(lines=lines)
        self.assertTrue(any("oracle output digest" in failure for failure in failures), failures)

    def test_rejects_changed_input_digest(self):
        lines = list(self.transcript)
        lines[1] = lines[1].replace(INPUT_DIGEST, "0000000000000000")
        failures, _ = self.verify(lines=lines)
        self.assertTrue(any("input digest" in failure for failure in failures), failures)

    def test_rejects_changed_case_count(self):
        lines = [self.transcript[0].replace("cases=2", "cases=3"),
                 self.transcript[1]]
        failures, _ = self.verify(lines=lines)
        self.assertTrue(any("case count" in failure for failure in failures), failures)

    def test_rejects_oracle_binary_outside_the_pinned_program(self):
        failures, _ = self.verify(oracle_sha="b" * 64)
        self.assertTrue(any("oracle DLL SHA-256" in failure for failure in failures), failures)

    def test_rejects_fixture_source_drift(self):
        (self.root / SOURCE).write_bytes(SOURCE_BYTES + b" changed")
        failures, _ = self.verify()
        self.assertTrue(any("fixture source SHA-256" in failure for failure in failures), failures)

    def test_fixture_source_hash_normalizes_crlf(self):
        source = self.root / SOURCE
        source.write_bytes(b"line one\nline two\n")
        lf_hash = oracle_proof.file_sha256(source)
        source.write_bytes(b"line one\r\nline two\r\n")
        self.assertEqual(oracle_proof.file_sha256(source), lf_hash)

    def test_rejects_missing_or_duplicate_proof_lines(self):
        for lines in (self.transcript[:1], self.transcript + [self.transcript[0]]):
            with self.subTest(lines=lines):
                failures, _ = self.verify(lines=lines)
                self.assertTrue(any("exactly one" in failure for failure in failures), failures)

    def joint_descriptor_baseline(self):
        source_bytes = b"fixed joint descriptor cases"
        source = self.root / "tests/PhysicsJointDescTests.cpp"
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_bytes(source_bytes)
        lines = [
            "case=0 actors a=null b=null in_anchor=3f800000.40000000.40400000 in_axis=3f000000.3f000000.3f000000",
            "before localNormal0=00000000.00000000.00000000 flags=00000000",
            "after_anchor localAnchor0=3f800000.40000000.40400000 flags=00000000",
            "after_axis localAxis0=00000000.3f800000.00000000 flags=00000000",
            "case=3 actors a=null b=null in_anchor=3f800000.00000000.00000000 in_axis=00000000.3f800000.00000000",
            "before localNormal0=00000000.00000000.00000000 flags=00000000",
            "after_anchor localAnchor0=3f800000.00000000.00000000 flags=00000000",
            "after_axis localAxis0=00000000.3f800000.00000000 flags=00000000",
        ]
        manifest = {
            "schema_version": 1,
            "targets": {
                "NxPhysicsJointDescTests": {
                    "format": "joint_descriptor",
                    "oracle_sha256": ORACLE_SHA,
                    "fixture_source": SOURCE.replace("PhysicsJointSupportTests", "PhysicsJointDescTests"),
                    "fixture_source_sha256": hashlib.sha256(source_bytes).hexdigest(),
                    "cases": 2,
                    "case_indices": [0, 3],
                    "input_digest_sha256": "77dc2aee3a82fae3b6f6a800ac25a617dfa78727c0858a74a8c1a46bb4766d45",
                    "oracle_output_digest_sha256": "bb6badb82de0eddb0cdbb227dcac6ca3582de7f65210e6d1b9b9fcd8ef82a331",
                }
            },
        }
        return manifest, lines

    def test_joint_descriptor_proof_hashes_inputs_and_oracle_results(self):
        manifest, lines = self.joint_descriptor_baseline()
        failures, proof = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsJointDescTests", lines, ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        self.assertEqual(proof["cases"], 2)
        self.assertEqual(proof["case_indices"], [0, 3])
        self.assertEqual(proof["input_digest_sha256"],
                         "77dc2aee3a82fae3b6f6a800ac25a617dfa78727c0858a74a8c1a46bb4766d45")
        self.assertEqual(proof["oracle_output_digest_sha256"],
                         "bb6badb82de0eddb0cdbb227dcac6ca3582de7f65210e6d1b9b9fcd8ef82a331")

    def test_joint_descriptor_proof_rejects_a_changed_oracle_result(self):
        manifest, lines = self.joint_descriptor_baseline()
        lines[2] = lines[2].replace("40400000", "40800000")
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsJointDescTests", lines, ORACLE_SHA, self.root)
        self.assertTrue(any("oracle output digest" in failure for failure in failures), failures)

    def test_joint_descriptor_proof_rejects_changed_case_set(self):
        manifest, lines = self.joint_descriptor_baseline()
        lines[4] = lines[4].replace("case=3", "case=0")
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsJointDescTests", lines, ORACLE_SHA, self.root)
        self.assertTrue(any("case indices" in failure for failure in failures), failures)


    def joint_matrix_baseline(self):
        source_bytes = b"fixed joint matrix fixture\n"
        source = self.root / "tests/PhysicsJointTests.cpp"
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_bytes(source_bytes)
        lines = [
            "pair_directory=C:\\oracle-a",
            "modules pair=2 trusted_system=10 rejected=0",
            "export=NxCreatePhysicsSDK present=yes",
            "case=hinge index=0 in_anchor=00000000.00000000.00000000 in_axis=3f800000.00000000.00000000",
            "case=d6 index=0 in motions=0.1.2.1.1.2",
            "case=pulley index=0 in_pulley0=00000000.40a00000.00000000 in_pulley1=40800000.40a00000.00000000 distance=40c00000 stiffness=3f400000 ratio=3fc00000 flags=00000001",
            "case=hinge index=0 created=yes",
            "rotated_fixture input=a row0=00000000.00000000.3f800000",
            "rotated_fixture actor=a t=00000000.00000000.00000000",
            "scene=released",
            "loaded module=NxPhysics.dll path=C:\\oracle-a\\NxPhysics.dll sha256=" + ORACLE_SHA,
        ]
        manifest = {
            "schema_version": 1,
            "targets": {
                "NxPhysicsJointTests": {
                    "format": "joint_matrix",
                    "oracle_sha256": ORACLE_SHA,
                    "fixture_source": "tests/PhysicsJointTests.cpp",
                    "fixture_source_sha256": hashlib.sha256(source_bytes).hexdigest(),
                    "cases": 1,
                    "input_lines": 4,
                    "case_family_counts": {"hinge": 1},
                    "oracle_output_lines": 4,
                    "input_digest_sha256": "7b5836f4001caaeb5642cf2fd81c1dc4e19c865035987d2916a8cfc65d07ff25",
                    "oracle_output_digest_sha256": "f6ead72f4dd8d072ddc0c29fc8f2c56c7b6797d995494506ac7c1603be7938ec",
                }
            },
        }
        return manifest, lines

    def test_joint_matrix_proof_hashes_fixed_inputs_and_oracle_outputs(self):
        manifest, lines = self.joint_matrix_baseline()
        failures, proof = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsJointTests", lines, ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        self.assertEqual(proof["cases"], 1)
        self.assertEqual(proof["input_lines"], 4)
        self.assertEqual(proof["case_family_counts"], {"hinge": 1})
        self.assertEqual(proof["oracle_output_lines"], 4)

    def test_joint_matrix_proof_ignores_loader_paths_but_rejects_changed_outputs(self):
        manifest, lines = self.joint_matrix_baseline()
        lines[0] = "pair_directory=Z:\\different-machine"
        lines[-1] = "loaded module=NxPhysics.dll path=Z:\\different-machine\\NxPhysics.dll sha256=" + ORACLE_SHA
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsJointTests", lines, ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        lines[6] = lines[6].replace("created=yes", "created=no")
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsJointTests", lines, ORACLE_SHA, self.root)
        self.assertTrue(any("oracle output digest" in failure for failure in failures), failures)

    def test_joint_matrix_proof_rejects_missing_input_case(self):
        manifest, lines = self.joint_matrix_baseline()
        lines.remove("case=hinge index=0 in_anchor=00000000.00000000.00000000 in_axis=3f800000.00000000.00000000")
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsJointTests", lines, ORACLE_SHA, self.root)
        self.assertTrue(any("input line count" in failure for failure in failures), failures)



class CheckedInOracleBaselineTests(unittest.TestCase):
    def test_baseline_is_registered_and_matches_the_oracle_coverage_contract(self):
        baseline_path = TOOLS.parent / "evidence" / "oracle-only-baselines.json"
        manifest = json.loads(baseline_path.read_text(encoding="utf-8"))
        phase_targets = test_gate_targets.phase_lists("NxPhaseOracleDifferentialTargets")
        registered = set(test_gate_targets.registered_targets(
            "NxRegisteredOracleDifferentialTargets"))
        coverage = test_gate_targets.registered_lines()

        for target, expected in manifest["targets"].items():
            phases = [phase for phase, targets in phase_targets.items() if target in targets]
            if target == "NxPhysicsJointSupportTests":
                self.assertEqual(phases, ["6", "7"],
                    "the shared joint-support oracle differential closes Phase 6 and remains in Phase 7")
            else:
                self.assertEqual(len(phases), 1, "%s must belong to exactly one phase" % target)
            self.assertIn(target, registered)
            lines = coverage[target]
            source_path = TOOLS.parents[3] / expected["fixture_source"]
            self.assertEqual(oracle_proof.file_sha256(source_path),
                             expected["fixture_source_sha256"])
            if expected.get("format", "joint_support") == "joint_support":
                output_prefix = "joint_support kind5 cases=%d oracle=%s " % (
                    expected["cases"], expected["oracle_output_digest_fnv64"])
                input_line = "joint_support inputs=%d digest=%s" % (
                    expected["cases"], expected["input_digest_fnv64"])
                self.assertEqual(sum(line.startswith(output_prefix) for line in lines), 1)
                self.assertEqual(sum(line == input_line for line in lines), 1)
            elif expected["format"] == "joint_descriptor":
                self.assertEqual(expected["case_indices"], [0, 3])
                self.assertEqual(len(expected["input_digest_sha256"]), 64)
                self.assertEqual(len(expected["oracle_output_digest_sha256"]), 64)
                self.assertEqual(expected["fixture_source"], "tests/PhysicsJointDescTests.cpp")
            else:
                self.assertEqual(expected["format"], "joint_matrix")
                self.assertEqual(expected["cases"], 122)
                self.assertEqual(expected["input_lines"], 284)
                self.assertEqual(sum(expected["case_family_counts"].values()), 122)
                self.assertEqual(expected["oracle_output_lines"], 2874)
                self.assertEqual(expected["fixture_source"], "tests/PhysicsJointTests.cpp")
                self.assertEqual(len(expected["input_digest_sha256"]), 64)
                self.assertEqual(len(expected["oracle_output_digest_sha256"]), 64)

    def test_retained_oracle_proofs_are_self_hashed_and_bound_to_their_baselines(self):
        baseline_path = TOOLS.parent / "evidence" / "oracle-only-baselines.json"
        proof_root = TOOLS.parent / "evidence" / "oracle-only-proofs"
        baseline = json.loads(baseline_path.read_text(encoding="utf-8"))
        proof_paths = sorted(proof_root.glob("*.json"))
        self.assertEqual({json.loads(path.read_text(encoding="utf-8"))["target"]
                          for path in proof_paths}, set(baseline["targets"]))
        baseline_hash = hashlib.sha256(json.dumps(
            baseline, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()

        for proof_path in proof_paths:
            with self.subTest(proof=proof_path.name):
                proof = json.loads(proof_path.read_text(encoding="utf-8"))
                expected_hash = proof.pop("proof_sha256")
                actual_hash = hashlib.sha256(json.dumps(
                    proof, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()
                expected = baseline["targets"][proof["target"]]

                self.assertEqual(actual_hash, expected_hash)
                self.assertEqual(proof["baseline_sha256"], baseline_hash)
                self.assertEqual(proof["oracle_sha256"], expected["oracle_sha256"])
                self.assertEqual(proof["fixture_source_sha256"], expected["fixture_source_sha256"])
                self.assertEqual(proof["cases"], expected["cases"])
                self.assertEqual(proof["format"], expected.get("format", "joint_support"))
                if proof["format"] == "joint_support":
                    self.assertEqual(proof["input_digest_fnv64"], expected["input_digest_fnv64"])
                    self.assertEqual(proof["oracle_output_digest_fnv64"],
                                     expected["oracle_output_digest_fnv64"])
                elif proof["format"] == "joint_descriptor":
                    self.assertEqual(proof["case_indices"], expected["case_indices"])
                    self.assertEqual(proof["input_digest_sha256"], expected["input_digest_sha256"])
                    self.assertEqual(proof["oracle_output_digest_sha256"],
                                     expected["oracle_output_digest_sha256"])
                else:
                    self.assertEqual(proof["input_lines"], expected["input_lines"])
                    self.assertEqual(proof["case_family_counts"], expected["case_family_counts"])
                    self.assertEqual(proof["oracle_output_lines"], expected["oracle_output_lines"])
                    self.assertEqual(proof["input_digest_sha256"], expected["input_digest_sha256"])
                    self.assertEqual(proof["oracle_output_digest_sha256"],
                                     expected["oracle_output_digest_sha256"])



if __name__ == "__main__":
    unittest.main()
