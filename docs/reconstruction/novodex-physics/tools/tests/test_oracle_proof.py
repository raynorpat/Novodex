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

    def test_rejects_missing_or_duplicate_proof_lines(self):
        for lines in (self.transcript[:1], self.transcript + [self.transcript[0]]):
            with self.subTest(lines=lines):
                failures, _ = self.verify(lines=lines)
                self.assertTrue(any("exactly one" in failure for failure in failures), failures)


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
            self.assertEqual(len(phases), 1, "%s must belong to exactly one phase" % target)
            self.assertIn(target, registered)
            lines = coverage[target]
            output_prefix = "joint_support kind5 cases=%d oracle=%s " % (
                expected["cases"], expected["oracle_output_digest_fnv64"])
            input_line = "joint_support inputs=%d digest=%s" % (
                expected["cases"], expected["input_digest_fnv64"])
            self.assertEqual(sum(line.startswith(output_prefix) for line in lines), 1)
            self.assertEqual(sum(line == input_line for line in lines), 1)

    def test_retained_phase7_proof_is_self_hashed_and_bound_to_its_baseline(self):
        baseline_path = TOOLS.parent / "evidence" / "oracle-only-baselines.json"
        proof_path = TOOLS.parent / "evidence" / "oracle-only-proofs" / "phase7-joint-support.json"
        baseline_bytes = baseline_path.read_bytes()
        baseline = json.loads(baseline_bytes)
        proof = json.loads(proof_path.read_text(encoding="utf-8"))
        expected_hash = proof.pop("proof_sha256")
        actual_hash = hashlib.sha256(json.dumps(
            proof, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()

        self.assertEqual(actual_hash, expected_hash)
        self.assertEqual(proof["baseline_sha256"], hashlib.sha256(
            json.dumps(baseline, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest())
        expected = baseline["targets"][proof["target"]]
        self.assertEqual(proof["oracle_sha256"], expected["oracle_sha256"])
        self.assertEqual(proof["fixture_source_sha256"], expected["fixture_source_sha256"])
        self.assertEqual(proof["cases"], expected["cases"])
        self.assertEqual(proof["input_digest_fnv64"], expected["input_digest_fnv64"])
        self.assertEqual(proof["oracle_output_digest_fnv64"],
                         expected["oracle_output_digest_fnv64"])


if __name__ == "__main__":
    unittest.main()
