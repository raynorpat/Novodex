"""Group the census's code rows into the translation units the linker laid out.

The image names 60 translation units through NX_ASSERT __FILE__ strings, and each
unit's code was emitted contiguously, so a unit's first and last asserting rows
bound an evidenced span. This tool grows each span outward one row at a time while
the next row has a direct call edge into the unit's current rows and none into the
neighbouring unit's. Rows neither side claims are published as the ambiguous rows
of a `gap:<left>..<right>` unit rather than handed to either neighbour.

The map describes the layout for scheduling work. It writes nothing into
inventory.json and does not change any row's phase.

Usage: python work_units.py --inventory inventory.json
           --ghidra oracle/ghidra/manifest.json
           --dependencies oracle/dependencies.dot --output work_units.json
"""

import argparse
import bisect
import json
import re
import sys
from collections import Counter
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parent))

from reconcile_analysis import (source_file_at, source_file_spans,
                                translation_unit_spans)  # noqa: E402


EDGE = re.compile(r'^\s*"(phys_fn_\d+)"\s*->\s*"(phys_fn_\d+)"')
START, END = "<start>", "<end>"


def hexa(value):
    return f"0x{value:08x}"


def load_edges(dot_text):
    edges = set()
    for line in dot_text.splitlines():
        match = EDGE.match(line)
        if match:
            edges.add((match.group(1), match.group(2)))
    return edges


def load_rows(inventory):
    rows = [{"id": r["id"], "rva": int(r["rva"], 16), "size": r["size"],
             "kind": r["kind"], "state": r["state"], "phase": r["phase"]}
            for r in inventory["functions"]]
    rows.sort(key=lambda r: r["rva"])
    return rows


def source_seeds(ghidra, rows):
    """Every row referencing a __FILE__ string names its own unit."""
    files = source_file_spans(ghidra)
    starts = [r["rva"] for r in rows]
    seeds = {}
    for reference in ghidra["references"]:
        if not reference["to_rva"]:
            continue
        name = source_file_at(files, int(reference["to_rva"], 16))
        if name is None:
            continue
        source = int(reference["from_rva"], 16)
        k = bisect.bisect_right(starts, source) - 1
        if k >= 0 and source < rows[k]["rva"] + rows[k]["size"]:
            seeds[rows[k]["rva"]] = name
    return seeds


def _joins(row_id, own, other, neighbours):
    near = neighbours.get(row_id, set())
    return bool(near & own) and not (near & other)


def _record(name, span, extent, ambiguous):
    counted = extent + ambiguous
    rows, size, phases = Counter(), Counter(), Counter()
    for r in counted:
        rows[r["state"]] += 1
        size[r["state"]] += r["size"]
        phases[str(r["phase"])] += 1
    return {"unit": name,
            "evidenced_span": [hexa(span[0]), hexa(span[1])] if span else None,
            "inferred_extent": [r["id"] for r in extent],
            "ambiguous_rows": [r["id"] for r in ambiguous],
            "rows": dict(sorted(rows.items())),
            "bytes": dict(sorted(size.items())),
            "phases": dict(sorted(phases.items())),
            "_first": counted[0]["rva"] if counted else (span[0] if span else 0)}


def build_units(rows, seeds, edges):
    code = [r for r in rows if r["kind"] == "code"]
    spans = translation_unit_spans(seeds)
    neighbours = {}
    for a, b in edges:
        neighbours.setdefault(a, set()).add(b)
        neighbours.setdefault(b, set()).add(a)

    members = {name: set() for _, _, name in spans}
    extent = {name: [] for _, _, name in spans}
    for r in code:
        for low, high, name in spans:
            if low <= r["rva"] <= high:
                members[name].add(r["id"])
                extent[name].append(r)
                break

    bounds = ([(START, None, -1)] + [(name, low, high) for low, high, name in spans]
              + [(END, float("inf"), None)])
    gaps = []
    for (left, _, left_high), (right, right_low, _) in zip(bounds, bounds[1:]):
        gap = [r for r in code if left_high < r["rva"] < right_low]
        i, j = 0, len(gap)
        if left in members:
            other = members.get(right, set())
            while i < j and _joins(gap[i]["id"], members[left], other, neighbours):
                members[left].add(gap[i]["id"])
                extent[left].append(gap[i])
                i += 1
        if right in members:
            other = members.get(left, set())
            while j > i and _joins(gap[j - 1]["id"], members[right], other, neighbours):
                members[right].add(gap[j - 1]["id"])
                extent[right].append(gap[j - 1])
                j -= 1
        if gap[i:j]:
            gaps.append((f"gap:{left}..{right}", gap[i:j]))

    units = [_record(name, (low, high), sorted(extent[name], key=lambda r: r["rva"]), [])
             for low, high, name in spans]
    units += [_record(name, None, [], rest) for name, rest in gaps]
    units.sort(key=lambda u: u.pop("_first"))

    seen = Counter(i for u in units for i in u["inferred_extent"] + u["ambiguous_rows"])
    missing = [r["id"] for r in code if seen[r["id"]] != 1]
    if missing:
        raise ValueError(f"{len(missing)} code rows are not in exactly one unit, "
                         f"first {missing[0]}")
    return units


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--inventory", required=True)
    parser.add_argument("--ghidra", required=True)
    parser.add_argument("--dependencies", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args(argv)
    try:
        inventory = json.loads(Path(args.inventory).read_text(encoding="utf-8"))
        ghidra = json.loads(Path(args.ghidra).read_text(encoding="utf-8"))
        dot = Path(args.dependencies).read_text(encoding="utf-8")
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    rows = load_rows(inventory)
    try:
        units = build_units(rows, source_seeds(ghidra, rows), load_edges(dot))
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    Path(args.output).write_text(
        json.dumps({"schema_version": 1, "units": units}, indent=1) + "\n",
        encoding="utf-8", newline="\n")
    named = [u for u in units if u["evidenced_span"]]
    print(f"units={len(units)} named={len(named)} gaps={len(units) - len(named)} "
          f"named_code_bytes={sum(sum(u['bytes'].values()) for u in named)} "
          f"ambiguous_rows={sum(len(u['ambiguous_rows']) for u in units)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
