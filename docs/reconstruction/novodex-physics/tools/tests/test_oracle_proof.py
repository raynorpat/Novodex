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

    def test_layout_digest_proof_pins_oracle_and_coverage_markers(self):
        source_bytes = b"fixed object layout fixture"
        source = self.root / "tests/PhysicsObjectLayoutTests.cpp"
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_bytes(source_bytes)
        manifest = {
            "schema_version": 1,
            "targets": {
                "NxPhysicsObjectLayoutTests": {
                    "format": "layout_digest",
                    "oracle_sha256": ORACLE_SHA,
                    "fixture_source": "tests/PhysicsObjectLayoutTests.cpp",
                    "fixture_source_sha256": hashlib.sha256(source_bytes).hexdigest(),
                    "oracle_digest_fnv32": "6a5f0123",
                    "coverage_line": "layout coverage tables=62 colobj=1 owner=1",
                }
            },
        }
        lines = [
            "layout coverage tables=62 colobj=1 owner=1",
            "layout oracle digest=6a5f0123",
            "layout candidate digest=ffffffff",
        ]

        failures, proof = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsObjectLayoutTests", lines, ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        self.assertEqual(proof["oracle_digest_fnv32"], "6a5f0123")
        self.assertEqual(proof["coverage_line"], "layout coverage tables=62 colobj=1 owner=1")

    def test_layout_digest_proof_rejects_changed_or_missing_oracle_markers(self):
        source_bytes = b"fixed object layout fixture"
        source = self.root / "tests/PhysicsObjectLayoutTests.cpp"
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_bytes(source_bytes)
        manifest = {
            "schema_version": 1,
            "targets": {
                "NxPhysicsObjectLayoutTests": {
                    "format": "layout_digest",
                    "oracle_sha256": ORACLE_SHA,
                    "fixture_source": "tests/PhysicsObjectLayoutTests.cpp",
                    "fixture_source_sha256": hashlib.sha256(source_bytes).hexdigest(),
                    "oracle_digest_fnv32": "6a5f0123",
                    "coverage_line": "layout coverage tables=62 colobj=1 owner=1",
                }
            },
        }
        good = [
            "layout coverage tables=62 colobj=1 owner=1",
            "layout oracle digest=6a5f0123",
        ]
        for changed in (
                [good[0], "layout oracle digest=6a5f0000"],
                ["layout coverage tables=61 colobj=1 owner=1", good[1]],
                [good[0], good[1], good[1]]):
            failures, _ = oracle_proof.verify_oracle_transcript(
                manifest, "NxPhysicsObjectLayoutTests", changed,
                ORACLE_SHA, self.root)
            self.assertTrue(failures, "changed layout evidence must be rejected")

    def test_fluid_oracle_proof_ignores_candidate_fields_and_pins_oracle_fields(self):
        source_bytes = b"fixed fluid emitter fixture"
        source = self.root / "tests/PhysicsFluidEmitterAbiTests.cpp"
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_bytes(source_bytes)
        normalized = (
            "fluid collision triangle-cells cases=3 oracle_digest=86d21a58 oracle_cells=14\n"
            "fluid license resolver cases=11 input_digest=04893d50 oracle=8d75d855\n"
            "fluid emitter owner/frame cases=2 oracle_owner=1 oracle_frame_actor=1\n"
            "implicit mesh secondary deleting-thunk oracle_slot=1\n")
        expected_digest = hashlib.sha256(normalized.encode("ascii")).hexdigest()
        manifest = {
            "schema_version": 1,
            "targets": {
                "NxPhysicsFluidEmitterAbiTests": {
                    "format": "fluid_oracle_lines",
                    "oracle_sha256": ORACLE_SHA,
                    "fixture_source": "tests/PhysicsFluidEmitterAbiTests.cpp",
                    "fixture_source_sha256": hashlib.sha256(source_bytes).hexdigest(),
                    "oracle_output_lines": 4,
                    "oracle_output_digest_sha256": expected_digest,
                }
            },
        }
        lines = [
            "fluid collision triangle-cells cases=3 oracle_digest=86d21a58 "
            "candidate_digest=86d21a58 oracle_cells=14 candidate_present=1 mismatches=0",
            "fluid license resolver cases=11 input_digest=04893d50 oracle=8d75d855 "
            "candidate=8d75d855 mismatches=0",
            "fluid emitter owner/frame cases=2 oracle_owner=1 candidate_owner=1 "
            "candidate_frame_actor=1 oracle_frame_actor=1 dll_owner=1 mismatches=0",
            "implicit mesh secondary deleting-thunk oracle_slot=1 candidate_slot=1 "
                "primary=1 secondary=1 record=1 return=1 stack=1 flag1_free=1 mismatches=0",
        ]

        failures, proof = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsFluidEmitterAbiTests", lines, ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        self.assertEqual(proof["oracle_output_lines"], 4)
        self.assertEqual(proof["oracle_output_digest_sha256"], expected_digest)

        candidate_changed = [
            lines[0].replace("candidate_digest=86d21a58", "candidate_digest=ffffffff")
                .replace("mismatches=0", "mismatches=17"),
            lines[1].replace("candidate=8d75d855", "candidate=00000000")
                .replace("mismatches=0", "mismatches=7"),
            lines[2].replace("candidate_owner=1", "candidate_owner=0")
                .replace("candidate_frame_actor=1", "candidate_frame_actor=0")
                .replace("dll_owner=1", "dll_owner=0"),
            lines[3].replace("candidate_slot=1", "candidate_slot=0")
                .replace("mismatches=0", "mismatches=4"),
        ]
        failures, candidate_proof = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsFluidEmitterAbiTests", candidate_changed,
            ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        self.assertEqual(candidate_proof["proof_sha256"], proof["proof_sha256"])

        changed_oracle = list(lines)
        changed_oracle[0] = changed_oracle[0].replace(
            "oracle_digest=86d21a58", "oracle_digest=00000000")
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsFluidEmitterAbiTests", changed_oracle,
            ORACLE_SHA, self.root)
        self.assertTrue(any("oracle output digest" in failure for failure in failures), failures)

        changed_row_oracle = list(lines)
        changed_row_oracle[3] = changed_row_oracle[3].replace(
            "oracle_slot=1", "oracle_slot=0")
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsFluidEmitterAbiTests", changed_row_oracle,
            ORACLE_SHA, self.root)
        self.assertTrue(any("oracle output digest" in failure for failure in failures), failures)

    def test_asset_oracle_proof_pins_multi_file_inputs_and_summary(self):
        source_names = [
            "tests/PhysicsAssetTests.cpp",
            "docs/reconstruction/novodex-physics/cases/assets/formats.json",
            "docs/reconstruction/novodex-physics/cases/assets/oracle-transcript.txt",
        ]
        source_bytes = {
            source_names[0]: b"fixed asset harness",
            source_names[1]: b"fixed asset fixture JSON",
            source_names[2]: b"fixed recorded oracle transcript",
        }
        for source_name, content in source_bytes.items():
            source = self.root / source_name
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_bytes(content)
        source_hashes = [(name, hashlib.sha256(source_bytes[name]).hexdigest())
                         for name in source_names]
        source_bundle = "".join("%s:%s\n" % item for item in source_hashes)
        source_digest = hashlib.sha256(source_bundle.encode("utf-8")).hexdigest()
        selected = "asset coverage driven=32 accepted=15 rejected=16 errors=10\nasset oracle digest=1daa095e expect_mismatches=0\n"
        output_digest = hashlib.sha256(selected.encode("ascii")).hexdigest()
        manifest = {
            "schema_version": 1,
            "targets": {
                "NxPhysicsAssetTests": {
                    "format": "asset_oracle_digest",
                    "oracle_sha256": ORACLE_SHA,
                    "fixture_sources": source_names,
                    "fixture_source_sha256": source_digest,
                    "cases": 32,
                    "oracle_output_digest_fnv32": "1daa095e",
                    "coverage_line": "asset coverage driven=32 accepted=15 rejected=16 errors=10",
                    "oracle_output_digest_sha256": output_digest,
                }
            },
        }
        lines = [
            "asset coverage driven=32 accepted=15 rejected=16 errors=10",
            "asset oracle digest=1daa095e expect_mismatches=0",
            "asset candidate mismatches=0 mode=differential",
        ]

        failures, proof = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsAssetTests", lines, ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        self.assertEqual(proof["cases"], 32)
        self.assertEqual(proof["oracle_output_digest_fnv32"], "1daa095e")
        self.assertEqual(proof["oracle_output_digest_sha256"], output_digest)
        self.assertEqual(proof["fixture_sources"], source_names)

        candidate_changed = list(lines)
        candidate_changed[2] = "asset candidate mismatches=99 mode=differential"
        failures, candidate_proof = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsAssetTests", candidate_changed, ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        self.assertEqual(candidate_proof["proof_sha256"], proof["proof_sha256"])

        changed_oracle = list(lines)
        changed_oracle[1] = changed_oracle[1].replace("1daa095e", "00000000")
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsAssetTests", changed_oracle, ORACLE_SHA, self.root)
        self.assertTrue(any("digest" in failure for failure in failures), failures)

        (self.root / source_names[1]).write_bytes(b"changed asset fixture JSON")
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsAssetTests", lines, ORACLE_SHA, self.root)
        self.assertTrue(any("fixture source SHA-256" in failure for failure in failures), failures)

    def test_thirdparty_oracle_proof_pins_generator_coverage_and_oracle_digest(self):
        source_bytes = b"fixed third-party generator and oracle call sequence"
        source_name = "tests/PhysicsThirdPartyTests.cpp"
        source = self.root / source_name
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_bytes(source_bytes)
        source_hash = hashlib.sha256(source_bytes).hexdigest()
        library_line = "thirdparty libraries qhull=2003.1 opcode=1.3-standalone"
        generator_line = "thirdparty generator=xorshift32 mode=self"
        coverage_lines = [
            "thirdparty coverage name=fixture cases=2 accepted=1 rejected=1",
            "thirdparty coverage driven=2 divergent=0 words=12 layout_checks=1",
        ]
        oracle_digest_line = "thirdparty oracle digest=1a2b3c4d"
        selected = "\n".join(
            [library_line, generator_line, *coverage_lines, oracle_digest_line]) + "\n"
        manifest = {
            "schema_version": 1,
            "targets": {
                "NxPhysicsThirdPartyTests": {
                    "format": "thirdparty_oracle_digest",
                    "oracle_sha256": ORACLE_SHA,
                    "fixture_source": source_name,
                    "fixture_source_sha256": source_hash,
                    "library_line": library_line,
                    "generator_line": generator_line,
                    "coverage_line_count": len(coverage_lines),
                    "final_coverage_line": coverage_lines[-1],
                    "oracle_digest_fnv32": "1a2b3c4d",
                    "selected_lines_sha256": hashlib.sha256(
                        selected.encode("ascii")).hexdigest(),
                }
            },
        }
        transcript = [
            "oracle module path=C:\\oracle\\NxPhysics.dll sha256=" + ORACLE_SHA,
            library_line,
            generator_line,
            *coverage_lines,
            oracle_digest_line,
            "thirdparty candidate digest=ffffffff",
        ]

        failures, proof = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsThirdPartyTests", transcript, ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        self.assertEqual(proof["coverage_line_count"], 2)
        self.assertEqual(proof["oracle_digest_fnv32"], "1a2b3c4d")

        changed_candidate = list(transcript)
        changed_candidate[-1] = "thirdparty candidate digest=00000000"
        failures, changed_proof = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsThirdPartyTests", changed_candidate,
            ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        self.assertEqual(changed_proof["proof_sha256"], proof["proof_sha256"])

        changed_coverage = list(transcript)
        changed_coverage[3] = changed_coverage[3].replace("cases=2", "cases=3")
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsThirdPartyTests", changed_coverage,
            ORACLE_SHA, self.root)
        self.assertTrue(any("coverage" in failure for failure in failures), failures)

        changed_oracle = list(transcript)
        changed_oracle[5] = "thirdparty oracle digest=00000000"
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsThirdPartyTests", changed_oracle,
            ORACLE_SHA, self.root)
        self.assertTrue(any("oracle digest" in failure for failure in failures), failures)

    def test_collision_oracle_proof_pins_inputs_coverage_and_oracle_outputs(self):
        source_name = "tests/PhysicsCollisionTests.cpp"
        source = self.root / source_name
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_bytes(b"fixed collision differential fixture")
        input_lines = [
            "collision input name=box.random words=12 input=0123456789abcdef",
            "collision input name=ray.aimed words=24 input=fedcba9876543210",
        ]
        coverage_lines = [
            "collision coverage name=box.random true=2 false=0 default_mismatches=0",
            "collision coverage name=ray.aimed hits=1 candidate_contacts=1 mismatches=0",
        ]
        output_lines = [
            "collision name=box index=0 checks=2 oracle=1111222233334444 candidate=ffffffffffffffff mismatches=0",
            "collision name=ray index=1 checks=1 oracle_true=1 candidate_contacts=1 mismatches=0",
        ]
        seeds = "collision seeds pair=c0ffee11 aimed=5eed10ad"
        selected = [
            seeds,
            *input_lines,
            "collision coverage name=box.random",
            "collision coverage name=ray.aimed",
            "collision name=box index=0 checks=2 oracle=1111222233334444",
            "collision name=ray index=1 checks=1 oracle_true=1",
        ]
        expected = {
            "format": "collision_oracle_digest",
            "oracle_sha256": ORACLE_SHA,
            "fixture_source": source_name,
            "fixture_source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
            "seed_line": seeds,
            "input_line_count": len(input_lines),
            "coverage_line_count": len(coverage_lines),
            "oracle_output_line_count": len(output_lines),
            "selected_lines_sha256": hashlib.sha256(
                ("\n".join(selected) + "\n").encode("ascii")).hexdigest(),
        }
        manifest = {"schema_version": 1,
                    "targets": {"NxPhysicsCollisionTests": expected}}
        transcript = [seeds, *input_lines, *coverage_lines, *output_lines]
        failures, proof = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsCollisionTests", transcript, ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        self.assertEqual(proof["input_line_count"], 2)
        self.assertEqual(proof["coverage_line_count"], 2)
        self.assertEqual(proof["oracle_output_line_count"], 2)

        candidate_changed = [line.replace("candidate=ffffffffffffffff", "candidate=0000000000000000")
                                  .replace("candidate_contacts=1", "candidate_contacts=99")
                                  .replace("mismatches=0", "mismatches=99")
                             for line in transcript]
        failures, candidate_proof = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsCollisionTests", candidate_changed, ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        self.assertEqual(candidate_proof["proof_sha256"], proof["proof_sha256"])

        changed_oracle = list(transcript)
        changed_oracle[-2] = changed_oracle[-2].replace(
            "1111222233334444", "0000222233334444")
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsCollisionTests", changed_oracle, ORACLE_SHA, self.root)
        self.assertTrue(any("selected oracle" in failure for failure in failures), failures)

        missing_coverage = list(transcript)
        missing_coverage.remove(coverage_lines[1])
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsCollisionTests", missing_coverage, ORACLE_SHA, self.root)
        self.assertTrue(any("coverage line count" in failure for failure in failures), failures)

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

    def test_shape_boxsweep_proof_hashes_the_oracle_input_and_output_digests(self):
        source_bytes = b"fixed box sweep inputs"
        source = self.root / "tests/PhysicsShapeVtableTests.cpp"
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_bytes(source_bytes)
        input_digest = "3e93b9a2"
        output_digest = "dd7822d1"
        manifest = {
            "schema_version": 1,
            "targets": {
                "NxPhysicsShapeVtableTests": {
                    "format": "shape_boxsweep",
                    "oracle_sha256": ORACLE_SHA,
                    "fixture_source": "tests/PhysicsShapeVtableTests.cpp",
                    "fixture_source_sha256": hashlib.sha256(source_bytes).hexdigest(),
                    "cases": 84,
                    "input_digest_fnv32": input_digest,
                    "oracle_output_digest_fnv32": output_digest,
                }
            },
        }
        lines = [
            "shape vtable boxsweep inputs=84 digest=%s" % input_digest,
            "shape vtable boxsweep oracle_digest=%s cases=84 failures=0" % output_digest,
        ]

        failures, proof = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsShapeVtableTests", lines, ORACLE_SHA, self.root)
        self.assertEqual(failures, [])
        self.assertEqual(proof["cases"], 84)
        self.assertEqual(proof["input_digest_fnv32"], input_digest)
        self.assertEqual(proof["oracle_output_digest_fnv32"], output_digest)

    def test_shape_boxsweep_proof_rejects_changed_oracle_data_or_case_count(self):
        source_bytes = b"fixed box sweep inputs"
        source = self.root / "tests/PhysicsShapeVtableTests.cpp"
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_bytes(source_bytes)
        manifest = {
            "schema_version": 1,
            "targets": {
                "NxPhysicsShapeVtableTests": {
                    "format": "shape_boxsweep",
                    "oracle_sha256": ORACLE_SHA,
                    "fixture_source": "tests/PhysicsShapeVtableTests.cpp",
                    "fixture_source_sha256": hashlib.sha256(source_bytes).hexdigest(),
                    "cases": 84,
                    "input_digest_fnv32": "3e93b9a2",
                    "oracle_output_digest_fnv32": "dd7822d1",
                }
            },
        }
        lines = [
            "shape vtable boxsweep inputs=84 digest=3e93b9a2",
            "shape vtable boxsweep oracle_digest=dd7822d1 cases=84 failures=0",
        ]
        for index, replacement in ((0, "00000000"), (1, "00000000")):
            changed = list(lines)
            if index == 0:
                changed[index] = changed[index].replace("3e93b9a2", replacement)
            else:
                changed[index] = changed[index].replace("dd7822d1", replacement)
            failures, _ = oracle_proof.verify_oracle_transcript(
                manifest, "NxPhysicsShapeVtableTests", changed, ORACLE_SHA, self.root)
            self.assertTrue(any("digest" in failure for failure in failures), failures)

        changed = list(lines)
        changed[1] = changed[1].replace("cases=84", "cases=83")
        failures, _ = oracle_proof.verify_oracle_transcript(
            manifest, "NxPhysicsShapeVtableTests", changed, ORACLE_SHA, self.root)
        self.assertTrue(any("case count" in failure for failure in failures), failures)

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
        self.assertIn("NxPhysicsObjectLayoutTests", manifest["targets"],
                      "Phase 5's object-layout oracle differential needs a pinned baseline")

        for target, expected in manifest["targets"].items():
            phases = [phase for phase, targets in phase_targets.items() if target in targets]
            if target == "NxPhysicsJointSupportTests":
                self.assertEqual(phases, ["3", "6", "7"],
                    "the shared joint-support oracle differential discharges Phase 2 in Phase 3 and remains in Phases 6-7")
            elif target == "NxPhysicsObjectLayoutTests":
                self.assertEqual(phases, ["5", "6", "7"],
                    "the object-layout oracle differential is shared by Phases 5-7")
            elif target == "NxPhysicsFluidEmitterAbiTests":
                self.assertEqual(phases, ["5", "7"],
                    "the fluid-emitter ABI oracle differential is shared by Phases 5 and 7")
            elif target == "NxPhysicsAssetTests":
                self.assertEqual(phases, ["4"], "asset oracle proof belongs to Phase 4")
            elif target == "NxPhysicsThirdPartyTests":
                self.assertEqual(phases, ["4", "7"], "third-party oracle proof is registered in Phases 4 and 7")
            elif target == "NxPhysicsCollisionTests":
                self.assertEqual(phases, ["3", "7"], "collision oracle proof is registered in Phases 3 and 7")
            else:
                self.assertEqual(len(phases), 1, "%s must belong to exactly one phase" % target)
            self.assertIn(target, registered)
            lines = coverage[target]
            if "fixture_sources" in expected:
                source_digest = oracle_proof.fixture_sources_sha256(
                    expected["fixture_sources"], TOOLS.parents[3])
            else:
                source_digest = oracle_proof.file_sha256(
                    TOOLS.parents[3] / expected["fixture_source"])
            self.assertEqual(source_digest, expected["fixture_source_sha256"])
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
            elif expected["format"] == "shape_boxsweep":
                self.assertEqual(expected["cases"], 84)
                self.assertEqual(expected["fixture_source"], "tests/PhysicsShapeVtableTests.cpp")
                self.assertEqual(len(expected["input_digest_fnv32"]), 8)
                self.assertEqual(len(expected["oracle_output_digest_fnv32"]), 8)
                self.assertEqual(sum(line.startswith("shape vtable boxsweep inputs=84 digest=")
                                      for line in lines), 1)
            elif expected["format"] == "layout_digest":
                fixture_sources = expected.get(
                    "fixture_sources", [expected.get("fixture_source")])
                self.assertTrue(set(fixture_sources) & {
                    "tests/PhysicsObjectLayoutTests.cpp",
                    "tests/PhysicsSceneAuxiliaryBlockTests.cpp",
                })
                self.assertEqual(len(expected["oracle_digest_fnv32"]), 8)
                self.assertEqual(sum(line == expected["coverage_line"] for line in lines), 1)
            elif expected["format"] == "fluid_oracle_lines":
                self.assertEqual(expected["fixture_source"], "tests/PhysicsFluidEmitterAbiTests.cpp")
                self.assertEqual(expected["oracle_output_lines"], 37)
                self.assertEqual(len(expected["oracle_output_digest_sha256"]), 64)
            elif expected["format"] == "asset_oracle_digest":
                self.assertEqual(expected["fixture_sources"], [
                    "tests/PhysicsAssetTests.cpp",
                    "docs/reconstruction/novodex-physics/cases/assets/formats.json",
                    "docs/reconstruction/novodex-physics/cases/assets/oracle-transcript.txt",
                ])
                self.assertEqual(expected["cases"], 32)
                self.assertEqual(expected["oracle_output_digest_fnv32"], "1daa095e")
                self.assertEqual(len(expected["oracle_output_digest_sha256"]), 64)
            elif expected["format"] == "thirdparty_oracle_digest":
                self.assertEqual(expected["fixture_source"], "tests/PhysicsThirdPartyTests.cpp")
                self.assertEqual(expected["generator_line"],
                                 "thirdparty generator=xorshift32 mode=differential")
                self.assertEqual(expected["coverage_line_count"], 27)
                self.assertEqual(expected["final_coverage_line"],
                                 "thirdparty coverage driven=116 divergent=26 words=2615055 layout_checks=47")
                self.assertEqual(expected["oracle_digest_fnv32"], "e2a266c1")
                self.assertEqual(len(expected["selected_lines_sha256"]), 64)
            elif expected["format"] == "collision_oracle_digest":
                self.assertEqual(expected["fixture_source"], "tests/PhysicsCollisionTests.cpp")
                self.assertEqual(expected["input_line_count"], 108)
                self.assertEqual(expected["coverage_line_count"], 135)
                self.assertEqual(expected["oracle_output_line_count"], 136)
                self.assertEqual(expected["seed_line"],
                                 "collision seeds pair=c0ffee11 aimed=5eed10ad")
                self.assertEqual(len(expected["selected_lines_sha256"]), 64)
            else:
                self.assertEqual(expected["format"], "joint_matrix")
                self.assertEqual(expected["cases"], 122)
                self.assertEqual(expected["input_lines"], 284)
                self.assertEqual(sum(expected["case_family_counts"].values()), 122)
                self.assertEqual(expected["oracle_output_lines"], 3234)
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
                if "cases" in expected:
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
                elif proof["format"] == "shape_boxsweep":
                    self.assertEqual(proof["input_digest_fnv32"],
                                     expected["input_digest_fnv32"])
                    self.assertEqual(proof["oracle_output_digest_fnv32"],
                                     expected["oracle_output_digest_fnv32"])
                elif proof["format"] == "layout_digest":
                    self.assertEqual(proof["oracle_digest_fnv32"],
                                     expected["oracle_digest_fnv32"])
                    self.assertEqual(proof["coverage_line"], expected["coverage_line"])
                elif proof["format"] == "fluid_oracle_lines":
                    self.assertEqual(proof["oracle_output_lines"],
                                     expected["oracle_output_lines"])
                    self.assertEqual(proof["oracle_output_digest_sha256"],
                                     expected["oracle_output_digest_sha256"])
                elif proof["format"] == "asset_oracle_digest":
                    self.assertEqual(proof["fixture_sources"], expected["fixture_sources"])
                    self.assertEqual(proof["cases"], expected["cases"])
                    self.assertEqual(proof["oracle_output_digest_fnv32"],
                                     expected["oracle_output_digest_fnv32"])
                    self.assertEqual(proof["oracle_output_digest_sha256"],
                                     expected["oracle_output_digest_sha256"])
                elif proof["format"] == "thirdparty_oracle_digest":
                    self.assertEqual(proof["coverage_line_count"],
                                     expected["coverage_line_count"])
                    self.assertEqual(proof["final_coverage_line"],
                                     expected["final_coverage_line"])
                    self.assertEqual(proof["oracle_digest_fnv32"],
                                     expected["oracle_digest_fnv32"])
                    self.assertEqual(proof["selected_lines_sha256"],
                                     expected["selected_lines_sha256"])
                elif proof["format"] == "collision_oracle_digest":
                    self.assertEqual(proof["input_line_count"], expected["input_line_count"])
                    self.assertEqual(proof["coverage_line_count"], expected["coverage_line_count"])
                    self.assertEqual(proof["oracle_output_line_count"],
                                     expected["oracle_output_line_count"])
                    self.assertEqual(proof["selected_lines_sha256"],
                                     expected["selected_lines_sha256"])
                else:
                    self.assertEqual(proof["input_lines"], expected["input_lines"])
                    self.assertEqual(proof["case_family_counts"], expected["case_family_counts"])
                    self.assertEqual(proof["oracle_output_lines"], expected["oracle_output_lines"])
                    self.assertEqual(proof["input_digest_sha256"], expected["input_digest_sha256"])
                    self.assertEqual(proof["oracle_output_digest_sha256"],
                                     expected["oracle_output_digest_sha256"])



if __name__ == "__main__":
    unittest.main()
