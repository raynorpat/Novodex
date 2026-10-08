#!/usr/bin/env python3
"""Generate a row-level Physics reconstruction backlog from pinned evidence."""

import argparse
import bisect
import hashlib
import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path


CONTROL_LITERAL = re.compile(r"(?<![0-9a-fA-F])0x([0-9a-fA-F]{4})(?![0-9a-fA-F])")


def _ledger_rows(ledgers):
    result = {}
    for ledger in ledgers:
        phase = ledger.get("phase")
        for disposition in ("closed", "deferred"):
            for entry in ledger.get(disposition, []):
                row_id = entry.get("id")
                if row_id:
                    result.setdefault(row_id, []).append({
                        "phase": phase,
                        "disposition": disposition,
                        "entry": entry,
                    })
    return result


def _matching_ledger(entries, phase):
    for entry in entries:
        if entry["phase"] == phase:
            return entry
    return entries[0] if entries else None


def _dependency_index(rows, capstone):
    if not capstone:
        return {}

    starts = []
    ranges = []
    row_at_start = {}
    for row in rows:
        try:
            start = int(row["rva"], 16)
            size = int(row["size"])
        except (KeyError, TypeError, ValueError):
            continue
        starts.append(start)
        ranges.append((start, start + size, row))
        row_at_start[start] = row
    ranges.sort(key=lambda item: item[0])
    starts = [item[0] for item in ranges]
    result = defaultdict(lambda: {
        "direct_rows": set(),
        "unresolved_direct_targets": set(),
        "unresolved_indirect_sites": [],
    })
    for row in rows:
        if row.get("kind") == "code":
            result[row["id"]]

    for instruction in capstone.get("instructions", []):
        if instruction.get("flow") != "call":
            continue
        try:
            call_rva = int(instruction["rva"], 16)
        except (KeyError, TypeError, ValueError):
            continue
        index = bisect.bisect_right(starts, call_rva) - 1
        if index < 0:
            continue
        start, end, owner = ranges[index]
        if not start <= call_rva < end or owner.get("kind") != "code":
            continue
        owner_calls = result[owner["id"]]
        if instruction.get("indirect"):
            owner_calls["unresolved_indirect_sites"].append(instruction.get("rva"))
            continue
        target = instruction.get("target_rva")
        try:
            target_rva = int(target, 16) if target else None
        except (TypeError, ValueError):
            target_rva = None
        target_row = row_at_start.get(target_rva)
        if target_row:
            if target_row["id"] != owner["id"]:
                owner_calls["direct_rows"].add(target_row["id"])
        elif target_rva is not None:
            owner_calls["unresolved_direct_targets"].add(f"0x{target_rva:08x}")

    return {
        row_id: {
            "direct_rows": sorted(value["direct_rows"]),
            "unresolved_direct_targets": sorted(value["unresolved_direct_targets"]),
            "unresolved_indirect_calls": len(value["unresolved_indirect_sites"]),
            "indirect_call_sites": sorted(
                rva for rva in value["unresolved_indirect_sites"] if rva is not None
            ),
            "basis": "Capstone direct calls only; indirect targets remain unresolved",
        }
        for row_id, value in result.items()
    }


def _implementation(row, repo_root):
    path = row.get("implementation")
    exists = None
    if path and repo_root is not None:
        exists = (repo_root / path).is_file()
    dynamic_text = row.get("dynamic_proof") or ""
    candidate_map_reference = "NxPhysics.map" in dynamic_text
    return {
        "path": path,
        "role": "reconstruction" if path else "unmapped",
        "symbol": row.get("implementation_symbol"),
        "file_exists": exists,
        "oracle_source": row.get("source"),
        "source_correspondence_recorded": bool(row.get("source")),
        "candidate_map_reference_recorded": candidate_map_reference,
        "candidate_map_reference_basis": "dynamic_proof" if candidate_map_reference else None,
    }


def _evidence(row, ledger_entry):
    closure = ledger_entry["entry"] if ledger_entry else None
    static_text = row.get("static_proof") or ""
    dynamic_text = row.get("dynamic_proof") or ""
    state = row.get("state")
    if closure and ledger_entry["disposition"] == "closed":
        strength = "row_specific_gate_falsification"
    elif state == "dynamically_gated":
        strength = "dynamic_gate_recorded"
    elif state == "statically_reviewed":
        strength = "static_review_recorded"
    elif static_text or dynamic_text:
        strength = "proof_text_recorded"
    elif state == "reconstructed":
        strength = "reconstruction_recorded"
    else:
        strength = "no_behavioral_proof_recorded"
    return {
        "strength": strength,
        "inventory_state": state,
        "static_proof_recorded": bool(static_text),
        "dynamic_proof_recorded": bool(dynamic_text),
        "ledger_phase": ledger_entry["phase"] if ledger_entry else None,
        "ledger_disposition": ledger_entry["disposition"] if ledger_entry else None,
        "ledger_reason": closure.get("reason") if closure else None,
        "gate": closure.get("gate") if closure else None,
        "falsification_recorded": bool(closure and closure.get("falsification")),
    }


def _next_packet(row, ledger_entry):
    state = row.get("state")
    phase = row.get("phase")
    row_id = row.get("id")
    if row.get("kind") != "code":
        return {
            "owner_phase": phase,
            "row_id": row_id,
            "action": "classified_artifact_review" if state != "classified" else None,
        }
    if state == "closed":
        action = "none"
        owner_phase = 8
    elif state == "discovered":
        action = "recover_contract_and_reconstruct"
        owner_phase = phase
    elif not row.get("implementation"):
        action = "record_candidate_implementation_mapping"
        owner_phase = phase
    elif state in ("typed", "decompiled"):
        action = "transcribe_and_drive_actual_dll"
        owner_phase = phase
    elif ledger_entry and ledger_entry["disposition"] == "closed":
        action = "full_census_reclosure"
        owner_phase = 8
    elif state in ("reconstructed", "statically_reviewed", "dynamically_gated"):
        action = "add_row_specific_falsification"
        owner_phase = phase
    else:
        action = "reconcile_inventory_state"
        owner_phase = phase
    entry = ledger_entry["entry"] if ledger_entry else None
    return {
        "owner_phase": owner_phase,
        "row_id": row_id,
        "rva": row.get("rva"),
        "action": action,
        "ledger_reason": entry.get("reason") if entry else None,
        "driving_phases": entry.get("driving_phases", []) if entry else [],
        "requires_dependency_review": True,
    }


def build_report(inventory, closure_ledgers, capstone=None, repo_root=None):
    all_rows = inventory.get("functions", [])
    code_rows = [row for row in all_rows if row.get("kind") == "code"]
    artifact_rows = [row for row in all_rows if row.get("kind") == "compiler_artifact"]
    exports = defaultdict(list)
    for export in inventory.get("exports", []):
        exports[export.get("function_id")].append(export.get("name"))
    ledger_index = _ledger_rows(closure_ledgers)
    dependencies = _dependency_index(all_rows, capstone or {})

    row_by_id = {row.get("id"): row for row in all_rows}
    report_rows = []
    phase_summary = defaultdict(lambda: {
        "rows": 0, "bytes": 0, "by_state": Counter(), "open_rows": 0,
        "rows_without_implementation_mapping": 0,
        "rows_with_unresolved_indirect_calls": 0,
    })
    for row in code_rows:
        ledger = _matching_ledger(ledger_index.get(row.get("id"), []), row.get("phase"))
        evidence = _evidence(row, ledger)
        implementation = _implementation(row, repo_root)
        reachability = {
            "status": "direct_export" if exports.get(row.get("id")) else "not_proven",
            "exports": sorted(name for name in exports.get(row.get("id"), []) if name),
            "closure_gate": evidence["gate"],
        }
        text = " ".join(str(row.get(field) or "") for field in
                        ("static_proof", "dynamic_proof", "notes"))
        contexts = {
            "four_digit_hex_literals": sorted(set(
                match.group(1).lower() for match in CONTROL_LITERAL.finditer(text)
            )),
            "simulation_mentioned": "simulat" in text.lower(),
            "interpretation": "textual evidence only; confirm context from the call path",
        }
        row_dependencies = dependencies.get(row["id"], {
            "direct_rows": [], "unresolved_direct_targets": [],
            "unresolved_indirect_calls": 0, "indirect_call_sites": [],
            "basis": "Capstone dependency data not available",
        })
        direct_call_rows = [
            {
                "id": dependency_id,
                "kind": row_by_id.get(dependency_id, {}).get("kind"),
                "state": row_by_id.get(dependency_id, {}).get("state"),
            }
            for dependency_id in row_dependencies["direct_rows"]
        ]
        unclosed_direct_callees = [
            dependency["id"] for dependency in direct_call_rows
            if dependency["kind"] == "code" and dependency["state"] != "closed"
        ]
        deferred_entry = ledger["entry"] if ledger and ledger["disposition"] == "deferred" else None
        item = {
            "id": row.get("id"),
            "rva": row.get("rva"),
            "size": row.get("size"),
            "phase": row.get("phase"),
            "kind": row.get("kind"),
            "state": row.get("state"),
            "label": row.get("label"),
            "implementation": implementation,
            "reachability": reachability,
            "dependencies": {
                **row_dependencies,
                "direct_call_rows": direct_call_rows,
            },
            "unresolved_dependencies": {
                "unclosed_direct_callees": unclosed_direct_callees,
                "unmapped_direct_targets": row_dependencies["unresolved_direct_targets"],
                "indirect_call_sites": row_dependencies["indirect_call_sites"],
                "schedule_blockers": deferred_entry.get("driving_phases", [])
                    if deferred_entry else [],
                "requires_virtual_dispatch_review": bool(
                    row_dependencies["unresolved_indirect_calls"]
                ),
            },
            "applicable_contexts": contexts,
            "evidence": evidence,
            "next_packet": _next_packet(row, ledger),
        }
        report_rows.append(item)

        phase = str(row.get("phase"))
        stats = phase_summary[phase]
        stats["rows"] += 1
        stats["bytes"] += int(row.get("size") or 0)
        stats["by_state"][row.get("state", "unknown")] += 1
        if row.get("state") != "closed":
            stats["open_rows"] += 1
        if not implementation["path"]:
            stats["rows_without_implementation_mapping"] += 1
        if row_dependencies["unresolved_indirect_calls"]:
            stats["rows_with_unresolved_indirect_calls"] += 1

    def row_totals(rows):
        return {"rows": len(rows), "bytes": sum(int(row.get("size") or 0) for row in rows)}

    artifact_states = Counter(row.get("state", "unknown") for row in artifact_rows)
    data_rows = inventory.get("data_objects", [])
    data_states = Counter(row.get("state", "unknown") for row in data_rows)
    intermediate_closure = {
        str(ledger.get("phase")): {
            "phase": ledger.get("phase"),
            "closed_rows": len(ledger.get("closed", [])),
            "deferred_rows": len(ledger.get("deferred", [])),
            "deferred_reasons": dict(sorted(Counter(
                row.get("reason", "unspecified") for row in ledger.get("deferred", [])
            ).items())),
        }
        for ledger in closure_ledgers
    }

    return {
        "schema_version": 1,
        "scope": "all code rows; artifact and data counts are separate summaries",
        "summary": {
            "function_rows": len(all_rows),
            "code_rows": len(code_rows),
            "code_bytes": row_totals(code_rows)["bytes"],
            "open_code_rows": sum(row.get("state") != "closed" for row in code_rows),
            "terminally_closed_code_rows": sum(
                row.get("state") == "closed" for row in code_rows
            ),
            "mapped_code_rows": sum(bool(row.get("implementation")) for row in code_rows),
            "source_correspondence_only_rows": sum(
                bool(row.get("source") and not row.get("implementation"))
                for row in code_rows
            ),
            "directly_exported_code_rows": sum(bool(exports.get(row.get("id")))
                                               for row in code_rows),
            "artifact_rows": len(artifact_rows),
            "artifact_bytes": row_totals(artifact_rows)["bytes"],
            "artifact_states": dict(sorted(artifact_states.items())),
            "data_objects": len(data_rows),
            "data_bytes": sum(int(row.get("size") or 0) for row in data_rows),
            "data_states": dict(sorted(data_states.items())),
            "phase_gate_status_from_inventory": [
                {"phase": row.get("phase"), "status": row.get("status")}
                for row in inventory.get("phases", [])
            ],
            "intermediate_closure_ledgers": intermediate_closure,
            "rows_with_unresolved_indirect_calls": sum(
                bool(dependencies.get(row["id"], {}).get("unresolved_indirect_calls"))
                for row in code_rows
            ),
            "phase_summary": {
                phase: {
                    "rows": stats["rows"],
                    "bytes": stats["bytes"],
                    "open_rows": stats["open_rows"],
                    "rows_without_implementation_mapping":
                        stats["rows_without_implementation_mapping"],
                    "rows_with_unresolved_indirect_calls":
                        stats["rows_with_unresolved_indirect_calls"],
                    "by_state": dict(sorted(stats["by_state"].items())),
                }
                for phase, stats in sorted(phase_summary.items(), key=lambda pair: int(pair[0]))
            },
        },
        "method": {
            "dependency_edges": "direct call targets mapped to inventory rows; indirect calls stay open",
            "reachability": "only named exports are promoted to direct public reachability",
            "contexts": "control-like literals and simulation mentions are cues, not static proof",
            "next_packet": "deterministic action from inventory state and the owning closure entry",
        },
        "rows": report_rows,
    }


def _sha256(path):
    # These fingerprints cover checked-in text files. Git may materialize them
    # with CRLF on Windows, so hash the canonical LF form for checkout-stable
    # snapshots.
    contents = path.read_bytes().replace(b"\r\n", b"\n")
    return hashlib.sha256(contents).hexdigest()


def write_report(path, report):
    """Write stable UTF-8/LF JSON bytes regardless of the host platform."""
    path.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                    encoding="utf-8", newline="\n")


def main(argv=None):
    script_root = Path(__file__).resolve().parents[4]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=script_root)
    parser.add_argument("--inventory", type=Path)
    parser.add_argument("--capstone", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)

    root = args.repo_root.resolve()
    inventory_path = (args.inventory or
                      root / "docs/reconstruction/novodex-physics/inventory.json").resolve()
    capstone_path = (args.capstone or
                     root / "docs/reconstruction/novodex-physics/oracle/capstone/manifest.json").resolve()
    output_path = (args.output or
                   root / "docs/reconstruction/novodex-physics/completion-backlog.json").resolve()
    try:
        inventory = json.loads(inventory_path.read_text(encoding="utf-8"))
        capstone = json.loads(capstone_path.read_text(encoding="utf-8"))
        closure_dir = root / "docs/reconstruction/novodex-physics/gates"
        ledgers = [json.loads(path.read_text(encoding="utf-8"))
                   for path in sorted(closure_dir.glob("phase*-closure.json"))]
    except (OSError, json.JSONDecodeError) as exc:
        print(f"completion report input error: {exc}", file=sys.stderr)
        return 2

    report = build_report(inventory, ledgers, capstone, root)
    report["source"] = {
        "inventory_sha256": _sha256(inventory_path),
        "capstone_sha256": _sha256(capstone_path),
        "generator_sha256": _sha256(Path(__file__).resolve()),
        "oracle_sha256": inventory.get("pins", {}).get("oracle", {}).get("sha256"),
        "closure_ledgers_sha256": {
            path.relative_to(root).as_posix(): _sha256(path)
            for path in sorted(closure_dir.glob("phase*-closure.json"))
        },
    }
    output_path.parent.mkdir(parents=True, exist_ok=True)
    write_report(output_path, report)
    summary = report["summary"]
    print(
        "completion_backlog=pass "
        f"code_rows={summary['code_rows']} code_bytes={summary['code_bytes']} "
        f"open_code_rows={summary['open_code_rows']} "
        f"artifact_rows={summary['artifact_rows']} artifact_bytes={summary['artifact_bytes']} "
        f"data_objects={summary['data_objects']} data_bytes={summary['data_bytes']} "
        f"output={output_path}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
