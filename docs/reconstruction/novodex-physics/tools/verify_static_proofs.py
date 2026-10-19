#!/usr/bin/env python3
"""Audit census closure, source mappings, static references, and evidence hashes."""

import argparse
import gzip
import hashlib
import json
import sys
from pathlib import Path, PurePosixPath


SIGNIFICANT_DATA_TYPES = frozenset({
    "code_addressed_global", "dispatch_table", "pointer_slot",
})
TERMINAL_STATE = {"code": "closed", "compiler_artifact": "classified"}


def _read_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8"))


def _finding(code, identity, **details):
    return {"code": code, "id": identity, **details}


def _source_path(value, source_root, repo_root):
    if not value or not isinstance(value, str):
        return None
    path = Path(value)
    candidates = [path] if path.is_absolute() else [Path(source_root) / path]
    if repo_root is not None and not path.is_absolute():
        candidates.append(Path(repo_root) / path)
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    return None


def _resolve_pointer(repo_root, reference, manifest_cache):
    if not isinstance(reference, str) or "#" not in reference:
        return False
    path_text, pointer = reference.split("#", 1)
    relative = Path(*PurePosixPath(path_text).parts)
    candidates = [Path(repo_root) / relative,
                  Path(repo_root) / "docs" / "reconstruction" / "novodex-physics" / relative]
    path = next((candidate for candidate in candidates if candidate.is_file()), candidates[0])
    if not path.is_file():
        return False
    cache_key = path.resolve()
    try:
        if cache_key not in manifest_cache:
            manifest_cache[cache_key] = _read_json(path)
        value = manifest_cache[cache_key]
        for segment in pointer.strip("/").split("/") if pointer else []:
            segment = segment.replace("~1", "/").replace("~0", "~")
            value = value[int(segment)] if isinstance(value, list) else value[segment]
    except (OSError, ValueError, KeyError, IndexError, TypeError):
        return False
    return True


def _verify_evidence_manifests(evidence_root):
    findings = []
    root = Path(evidence_root)
    if not root.is_dir():
        return [_finding("evidence_root_missing", str(root))]
    for manifest_path in sorted(root.rglob("manifest.json")):
        try:
            manifest = _read_json(manifest_path)
        except (OSError, ValueError) as error:
            findings.append(_finding("evidence_manifest_invalid",
                                     manifest_path.relative_to(root).as_posix(),
                                     detail=str(error)))
            continue
        entries = manifest.get("files", {})
        if isinstance(entries, list):
            normalized = {}
            for entry in entries:
                entry_path = entry.get("path") or entry.get("file") \
                    if isinstance(entry, dict) else None
                if isinstance(entry, dict) and isinstance(entry_path, str):
                    normalized[entry_path] = entry
                elif isinstance(entry, str):
                    normalized[entry] = None
                else:
                    findings.append(_finding("evidence_manifest_invalid",
                                             manifest_path.relative_to(root).as_posix(),
                                             detail="file list entry has no path"))
            entries = normalized
        if not isinstance(entries, dict):
            findings.append(_finding("evidence_manifest_invalid",
                                     manifest_path.relative_to(root).as_posix(),
                                     detail="files must be an object or path list"))
            continue
        packet_root = manifest_path.parent.resolve()
        for name, expected in sorted(entries.items()):
            relative = PurePosixPath(name)
            if relative.is_absolute() or ".." in relative.parts:
                findings.append(_finding("evidence_manifest_path_invalid", name))
                continue
            path = packet_root.joinpath(*relative.parts)
            identity = path.relative_to(root.resolve()).as_posix() if path.is_relative_to(root.resolve()) else name
            expected_hash = expected if isinstance(expected, str) else None
            hash_mode = "raw"
            if isinstance(expected, dict):
                if name.lower().endswith(".gz"):
                    expected_hash = (expected.get("compressed_sha256") or
                                     expected.get("sha256_gzip") or
                                     expected.get("sha256"))
                    if expected.get("compressed_sha256") or expected.get("sha256_gzip"):
                        hash_mode = "raw"
                    elif expected.get("uncompressed_sha256") or expected.get("sha256_uncompressed"):
                        if expected.get("sha256"):
                            expected_hash = expected["sha256"]
                            hash_mode = "raw"
                        else:
                            expected_hash = (expected.get("uncompressed_sha256") or
                                             expected.get("sha256_uncompressed"))
                            hash_mode = "gzip-uncompressed"
                    elif expected.get("source_bytes") is not None:
                        hash_mode = "gzip-uncompressed"
                    else:
                        # Older retained bundles use `sha256` for stored gzip bytes.
                        hash_mode = "raw"
                else:
                    expected_hash = expected.get("sha256")
            if not isinstance(expected_hash, str):
                findings.append(_finding("evidence_manifest_missing_hash", identity))
                continue
            try:
                raw_bytes = path.read_bytes()
                data = gzip.decompress(raw_bytes) if hash_mode == "gzip-uncompressed" else raw_bytes
                actual = hashlib.sha256(data).hexdigest()
            except OSError:
                findings.append(_finding("evidence_file_missing", identity))
                continue
            if actual != expected_hash and not name.lower().endswith(".gz"):
                try:
                    normalized_text_hash = hashlib.sha256(
                        raw_bytes.replace(b"\r\n", b"\n")).hexdigest()
                except UnboundLocalError:
                    normalized_text_hash = None
                if normalized_text_hash == expected_hash:
                    continue
            if actual != expected_hash:
                findings.append(_finding("evidence_hash_mismatch", identity,
                                         expected=expected_hash, actual=actual))
    return findings


def _validate_dispatch_and_fields(dispatch_map, field_map, findings):
    if dispatch_map is None:
        findings.append(_finding("vtable_map_missing", "dispatch-map.json"))
    else:
        known_ids = set(dispatch_map.get("function_ids", []))
        for slot in dispatch_map.get("slots", []):
            function_id = slot.get("function_id")
            if not slot.get("known", False) or (function_id and function_id not in known_ids):
                findings.append(_finding("unknown_vtable_slot", slot.get("id", "unknown"),
                                         function_id=function_id))
    if field_map is None:
        findings.append(_finding("field_map_missing", "field-map.json"))
    else:
        for use in field_map.get("uses", []):
            if not use.get("explained", False):
                findings.append(_finding("unexplained_field_use", use.get("id", "unknown"),
                                         function_id=use.get("function_id")))


def audit_inventory(inventory, source_root, repo_root, evidence_root=None,
                    dispatch_map=None, field_map=None):
    """Return an actionable audit report and a census-to-source crosswalk."""
    findings = []
    source_rows = []
    reference_cache = {}
    functions = inventory.get("functions", [])
    function_ids = {row.get("id") for row in functions}

    for row in functions:
        row_id = row.get("id", "unknown")
        kind = row.get("kind")
        terminal = TERMINAL_STATE.get(kind)
        if terminal is None:
            findings.append(_finding("function_kind_unknown", row_id, kind=kind))
        elif row.get("state") != terminal:
            code = "code_row_not_closed" if kind == "code" else "artifact_not_classified"
            findings.append(_finding(code, row_id, state=row.get("state"), expected=terminal))

        if kind == "code":
            implementation = row.get("implementation")
            symbol = row.get("implementation_symbol")
            path = _source_path(implementation, source_root, repo_root)
            if path is None or not symbol:
                findings.append(_finding("source_mapping_missing", row_id,
                                         implementation=implementation,
                                         implementation_symbol=symbol))
            else:
                relative = path.relative_to(Path(source_root).resolve()).as_posix() \
                    if path.is_relative_to(Path(source_root).resolve()) else path.as_posix()
                source_rows.append({"id": row_id, "rva": row.get("rva"),
                                    "path": relative, "symbol": symbol})
            for ref_name in ("ghidra_ref", "capstone_ref"):
                reference = row.get(ref_name)
                if not reference:
                    findings.append(_finding(f"{ref_name.removesuffix('_ref')}_reference_missing",
                                             row_id))
                elif not _resolve_pointer(repo_root, reference, reference_cache):
                    findings.append(_finding(f"{ref_name.removesuffix('_ref')}_reference_invalid",
                                             row_id, reference=reference))
            if not row.get("static_proof"):
                findings.append(_finding("static_proof_missing", row_id))
        elif kind == "compiler_artifact":
            proof = row.get("static_proof")
            if not isinstance(proof, str) or not proof.strip():
                findings.append(_finding("artifact_classification_missing", row_id))

    for row in inventory.get("data_objects", []):
        row_id = row.get("id", "unknown")
        if row.get("state") != "classified":
            findings.append(_finding("data_object_not_classified", row_id,
                                     state=row.get("state")))
        if row.get("type") in SIGNIFICANT_DATA_TYPES and not row.get("owner"):
            findings.append(_finding("significant_data_unowned", row_id,
                                     data_type=row.get("type")))
        proof = row.get("structural_proof")
        generic_data_claims = {
            "ghidra typed this bytes as data",
            "ghidra typed these bytes as data",
        }
        if (not isinstance(proof, str) or not proof.strip() or
                proof.strip().lower() in generic_data_claims):
            findings.append(_finding("data_structure_unproven", row_id))

    if evidence_root is not None:
        findings.extend(_verify_evidence_manifests(evidence_root))
    _validate_dispatch_and_fields(dispatch_map, field_map, findings)

    source_rows.sort(key=lambda row: (row["path"], row["symbol"], row["id"]))
    by_source = {}
    for row in source_rows:
        by_source.setdefault(row["path"], []).append(row["id"])
    source_map = {
        "schema_version": 1,
        "source_files": [
            {"path": path, "function_ids": ids}
            for path, ids in sorted(by_source.items())
        ],
        "functions": source_rows,
        "vtable_slots": [] if dispatch_map is None else dispatch_map.get("slots", []),
        "field_uses": [] if field_map is None else field_map.get("uses", []),
    }
    findings.sort(key=lambda row: (row["code"], row["id"]))
    report = {
        "schema_version": 1,
        "summary": {
            "function_rows": len(functions),
            "data_objects": len(inventory.get("data_objects", [])),
            "findings": len(findings),
            "source_mapped_code_rows": len(source_rows),
            "distinct_source_files": len(by_source),
        },
        "analysis_inputs": {
            "reference_manifests_loaded": len(reference_cache),
        },
        "findings": findings,
    }
    return report, source_map


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", required=True)
    parser.add_argument("--source-root", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--repo-root")
    parser.add_argument("--evidence-root")
    parser.add_argument("--dispatch-map")
    parser.add_argument("--field-map")
    args = parser.parse_args(argv)

    inventory_path = Path(args.inventory).resolve()
    repo_root = Path(args.repo_root).resolve() if args.repo_root else inventory_path.parents[3]
    evidence_root = (Path(args.evidence_root).resolve() if args.evidence_root else
                     repo_root / "docs" / "reconstruction" / "novodex-physics" / "evidence")
    try:
        inventory = _read_json(inventory_path)
        dispatch_map = _read_json(args.dispatch_map) if args.dispatch_map else None
        field_map = _read_json(args.field_map) if args.field_map else None
        report, source_map = audit_inventory(
            inventory, args.source_root, repo_root, evidence_root,
            dispatch_map, field_map)
        output_path = Path(args.output)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(json.dumps(report, indent=2) + "\n",
                               encoding="utf-8", newline="\n")
        source_map_path = output_path.with_name("source-map.json")
        source_map_path.write_text(json.dumps(source_map, indent=2) + "\n",
                                   encoding="utf-8", newline="\n")
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    print(f"static_proof=pass findings=0" if not report["findings"] else
          f"static_proof=fail findings={report['summary']['findings']}")
    print(f"source_mapped_code_rows={report['summary']['source_mapped_code_rows']}")
    print(f"source_files={report['summary']['distinct_source_files']}")
    return 0 if not report["findings"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
