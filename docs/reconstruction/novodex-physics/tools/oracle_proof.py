#!/usr/bin/env python3
"""Verify and retain an oracle-only input/output proof for a registered test."""

import argparse
import hashlib
import json
import pathlib
import re
import sys


KIND5_LINE = re.compile(
    r"^joint_support kind5 cases=(\d+) oracle=([0-9a-f]{16}) "
    r"candidate=[0-9a-f]{16} mismatches=\d+$")
INPUT_LINE = re.compile(r"^joint_support inputs=(\d+) digest=([0-9a-f]{16})$")


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def file_sha256(path):
    return sha256(pathlib.Path(path).read_bytes())


def verify_oracle_transcript(manifest, target, transcript_lines, oracle_sha256,
                             repo_root):
    """Return (failures, proof); candidate-only values are intentionally ignored."""
    failures = []
    targets = manifest.get("targets", {}) if manifest.get("schema_version") == 1 else {}
    expected = targets.get(target)
    if expected is None:
        return ["target has no pinned oracle proof: %s" % target], None

    if oracle_sha256.lower() != expected.get("oracle_sha256", "").lower():
        failures.append("oracle DLL SHA-256 differs from the pinned baseline")

    source_name = expected.get("fixture_source", "")
    source_rel = pathlib.PurePosixPath(source_name)
    if not source_name or source_rel.is_absolute() or ".." in source_rel.parts:
        failures.append("fixture source path is invalid")
        source_digest = None
    else:
        source_path = pathlib.Path(repo_root).joinpath(*source_rel.parts)
        if not source_path.is_file():
            failures.append("fixture source is missing: %s" % source_name)
            source_digest = None
        else:
            source_digest = file_sha256(source_path)
            if source_digest != expected.get("fixture_source_sha256"):
                failures.append("fixture source SHA-256 differs from the pinned baseline")

    kind_lines, input_lines = [], []
    for line in transcript_lines:
        line = line.rstrip("\r\n")
        if line.startswith("joint_support kind5 "):
            kind_lines.append(line)
        elif line.startswith("joint_support inputs="):
            input_lines.append(line)
    if len(kind_lines) != 1:
        failures.append("expected exactly one joint_support kind5 proof line; found %d"
                        % len(kind_lines))
    if len(input_lines) != 1:
        failures.append("expected exactly one joint_support input proof line; found %d"
                        % len(input_lines))

    kind_match = KIND5_LINE.fullmatch(kind_lines[0]) if len(kind_lines) == 1 else None
    input_match = INPUT_LINE.fullmatch(input_lines[0]) if len(input_lines) == 1 else None
    if len(kind_lines) == 1 and kind_match is None:
        failures.append("joint_support kind5 proof line is malformed")
    if len(input_lines) == 1 and input_match is None:
        failures.append("joint_support input proof line is malformed")

    cases = input_cases = None
    input_digest = oracle_output_digest = None
    if kind_match:
        cases = int(kind_match.group(1))
        oracle_output_digest = kind_match.group(2)
        if cases != expected.get("cases"):
            failures.append("oracle case count differs from the pinned baseline")
        if oracle_output_digest != expected.get("oracle_output_digest_fnv64"):
            failures.append("oracle output digest differs from the pinned baseline")
    if input_match:
        input_cases = int(input_match.group(1))
        input_digest = input_match.group(2)
        if input_cases != expected.get("cases") or input_cases != cases:
            failures.append("input case count differs from the pinned baseline")
        if input_digest != expected.get("input_digest_fnv64"):
            failures.append("input digest differs from the pinned baseline")

    proof = None
    if kind_match and input_match and source_digest:
        proof = {
            "schema_version": 1,
            "target": target,
            "oracle_sha256": oracle_sha256.lower(),
            "fixture_source": source_name,
            "fixture_source_sha256": source_digest,
            "cases": cases,
            "input_digest_fnv64": input_digest,
            "oracle_output_digest_fnv64": oracle_output_digest,
            "proof_lines_sha256": sha256((kind_lines[0] + "\n" + input_lines[0] + "\n").encode("ascii")),
            "baseline_sha256": sha256(json.dumps(
                manifest, sort_keys=True, separators=(",", ":")).encode("utf-8")),
        }
        proof["proof_sha256"] = sha256(json.dumps(
            proof, sort_keys=True, separators=(",", ":")).encode("utf-8"))
    return failures, proof


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--target", required=True)
    parser.add_argument("--transcript", required=True)
    parser.add_argument("--oracle-sha256", required=True)
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--evidence-output", required=True)
    args = parser.parse_args(argv)

    try:
        manifest_path = pathlib.Path(args.manifest)
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        transcript = pathlib.Path(args.transcript).read_text(encoding="utf-8-sig").splitlines()
        failures, proof = verify_oracle_transcript(
            manifest, args.target, transcript, args.oracle_sha256, args.repo_root)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        sys.stderr.write("oracle proof failed to read its inputs: %s\n" % error)
        return 2

    if failures:
        for failure in failures:
            sys.stderr.write("oracle proof FAILED %s\n" % failure)
        return 1

    output = pathlib.Path(args.evidence_output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    sys.stdout.write("oracle proof pass target=%s cases=%d input_digest=%s "
                     "oracle_output_digest=%s proof_sha256=%s evidence=%s\n" %
                     (proof["target"], proof["cases"], proof["input_digest_fnv64"],
                      proof["oracle_output_digest_fnv64"], proof["proof_sha256"], output))
    return 0


if __name__ == "__main__":
    sys.exit(main())
