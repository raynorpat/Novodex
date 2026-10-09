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
JOINT_DESC_CASE = re.compile(r"^case=(\d+) actors .+ in_anchor=[0-9a-f]{8}(?:\.[0-9a-f]{8}){2} in_axis=[0-9a-f]{8}(?:\.[0-9a-f]{8}){2}$")
JOINT_DESC_OUTPUT_PREFIXES = ("before ", "after_anchor ", "after_axis ")
JOINT_MATRIX_INPUT_CASE = re.compile(
    r"^case=([a-z0-9_]+) index=(\d+) .*?\bin_anchor=")
JOINT_MATRIX_SUPPLEMENTAL_INPUT = re.compile(r"^case=(?:pulley index=\d+ in_pulley0=|d6 index=\d+ in )")
JOINT_MATRIX_MACHINE_PREFIXES = ("pair_directory=", "modules pair=", "loaded module=")


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def file_sha256(path):
    # Fixture source is text and checkouts may materialize LF or CRLF. Pin the
    # logical source bytes so a proof remains valid across those worktrees.
    return sha256(pathlib.Path(path).read_bytes().replace(b"\r\n", b"\n"))


def _verify_joint_support(expected, lines):
    failures = []
    kind_lines = [line for line in lines if line.startswith("joint_support kind5 ")]
    input_lines = [line for line in lines if line.startswith("joint_support inputs=")]
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

    if not kind_match or not input_match:
        return failures, None
    return failures, {
        "cases": cases,
        "input_digest_fnv64": input_digest,
        "oracle_output_digest_fnv64": oracle_output_digest,
        "selected_lines": [kind_lines[0], input_lines[0]],
    }


def _verify_joint_descriptor(expected, lines):
    failures = []
    case_lines = [line for line in lines if line.startswith("case=")]
    case_matches = [JOINT_DESC_CASE.fullmatch(line) for line in case_lines]
    if any(match is None for match in case_matches):
        failures.append("joint descriptor case input line is malformed")
    case_indices = [int(match.group(1)) for match in case_matches if match]
    if case_indices != expected.get("case_indices"):
        failures.append("joint descriptor case indices differ from the pinned baseline")
    if len(case_indices) != expected.get("cases"):
        failures.append("oracle case count differs from the pinned baseline")

    output_lines = [line for line in lines
                    if line.startswith(JOINT_DESC_OUTPUT_PREFIXES)]
    if len(output_lines) != len(case_indices) * len(JOINT_DESC_OUTPUT_PREFIXES):
        failures.append("joint descriptor output count does not match its cases")
    for offset in range(0, len(output_lines), len(JOINT_DESC_OUTPUT_PREFIXES)):
        stages = tuple(line.split(" ", 1)[0] for line in
                       output_lines[offset:offset + len(JOINT_DESC_OUTPUT_PREFIXES)])
        if stages != tuple(stage.strip() for stage in JOINT_DESC_OUTPUT_PREFIXES):
            failures.append("joint descriptor output stage order changed")
            break

    input_digest = sha256(("\n".join(case_lines) + "\n").encode("ascii")) if case_lines else None
    oracle_output_digest = sha256(("\n".join(output_lines) + "\n").encode("ascii")) if output_lines else None
    if input_digest != expected.get("input_digest_sha256"):
        failures.append("input digest differs from the pinned baseline")
    if oracle_output_digest != expected.get("oracle_output_digest_sha256"):
        failures.append("oracle output digest differs from the pinned baseline")
    selected_lines = [line for line in lines
                      if line.startswith("case=") or line.startswith(JOINT_DESC_OUTPUT_PREFIXES)]
    if failures:
        return failures, None
    return failures, {
        "cases": len(case_indices),
        "case_indices": case_indices,
        "input_digest_sha256": input_digest,
        "oracle_output_digest_sha256": oracle_output_digest,
        "selected_lines": selected_lines,
    }


def _verify_joint_matrix(expected, lines):
    failures = []
    selected_lines = [line for line in lines
                      if not line.startswith(JOINT_MATRIX_MACHINE_PREFIXES)]
    input_lines = []
    case_input_lines = []
    case_family_counts = {}
    for line in selected_lines:
        match = JOINT_MATRIX_INPUT_CASE.match(line)
        if match:
            input_lines.append(line)
            case_input_lines.append(line)
            family = match.group(1)
            case_family_counts[family] = case_family_counts.get(family, 0) + 1
        elif JOINT_MATRIX_SUPPLEMENTAL_INPUT.match(line):
            input_lines.append(line)
        elif line.startswith("rotated_fixture input=") or (
                line.startswith("posed_fixture=") and " input=" in line):
            input_lines.append(line)

    output_lines = [line for line in selected_lines if line not in input_lines]
    if len(case_input_lines) != expected.get("cases"):
        failures.append("joint matrix case count differs from the pinned baseline")
    if len(input_lines) != expected.get("input_lines"):
        failures.append("joint matrix input line count differs from the pinned baseline")
    if case_family_counts != expected.get("case_family_counts"):
        failures.append("joint matrix case-family counts differ from the pinned baseline")
    if len(output_lines) != expected.get("oracle_output_lines"):
        failures.append("joint matrix oracle output line count differs from the pinned baseline")

    input_digest = sha256(("\n".join(input_lines) + "\n").encode("ascii")) if input_lines else None
    oracle_output_digest = sha256(("\n".join(output_lines) + "\n").encode("ascii")) if output_lines else None
    if input_digest != expected.get("input_digest_sha256"):
        failures.append("input digest differs from the pinned baseline")
    if oracle_output_digest != expected.get("oracle_output_digest_sha256"):
        failures.append("oracle output digest differs from the pinned baseline")
    if failures:
        return failures, None
    return failures, {
        "cases": len(case_input_lines),
        "input_lines": len(input_lines),
        "case_family_counts": case_family_counts,
        "oracle_output_lines": len(output_lines),
        "input_digest_sha256": input_digest,
        "oracle_output_digest_sha256": oracle_output_digest,
        "selected_lines": selected_lines,
    }


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

    lines = [line.rstrip("\r\n") for line in transcript_lines]
    proof_format = expected.get("format", "joint_support")
    if proof_format == "joint_support":
        failures_for_format, measurements = _verify_joint_support(expected, lines)
    elif proof_format == "joint_descriptor":
        failures_for_format, measurements = _verify_joint_descriptor(expected, lines)
    elif proof_format == "joint_matrix":
        failures_for_format, measurements = _verify_joint_matrix(expected, lines)
    else:
        failures_for_format, measurements = ["unsupported oracle proof format: %s"
                                               % proof_format], None
    failures.extend(failures_for_format)

    proof = None
    if measurements is not None and source_digest:
        selected_text = "\n".join(measurements.pop("selected_lines")) + "\n"
        proof = {
            "schema_version": 1,
            "target": target,
            "format": proof_format,
            "oracle_sha256": oracle_sha256.lower(),
            "fixture_source": source_name,
            "fixture_source_sha256": source_digest,
            "proof_lines_sha256": sha256(selected_text.encode("ascii")),
            "baseline_sha256": sha256(json.dumps(
                manifest, sort_keys=True, separators=(",", ":")).encode("utf-8")),
            **measurements,
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
    input_digest = proof.get("input_digest_fnv64", proof.get("input_digest_sha256"))
    output_digest = proof.get("oracle_output_digest_fnv64",
                              proof.get("oracle_output_digest_sha256"))
    sys.stdout.write("oracle proof pass target=%s cases=%d input_digest=%s "
                     "oracle_output_digest=%s proof_sha256=%s evidence=%s\n" %
                     (proof["target"], proof["cases"], input_digest,
                      output_digest, proof["proof_sha256"], output))
    return 0


if __name__ == "__main__":
    sys.exit(main())
