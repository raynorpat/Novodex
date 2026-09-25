# Translation-Unit Work Units and Revolute Joint Pilot — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build tooling that groups census rows by original translation unit, then write real product source for the revolute joint's two translation units (plus the shared joint base rows they need), wire it into scene joint creation, and measure the cost.

**Architecture:** Two Python tools read committed oracle data: `work_units.py` partitions code rows into units, and `unit_bundle.py` writes a per-unit reference document. A pinned Ghidra headless script fills in decompiles for rows Ghidra never made functions. The pilot adds `Physics/src/core/` sources mirroring the original files. Verification is regression-only: the existing joint staged-pair transcript must stay byte-identical once revolute creation runs the new code.

**Tech Stack:** Python 3.13 (`unittest`), Ghidra 12.1.2 headless (Java GhidraScript), MSVC Win32 Release via CMake (`Visual Studio 18 2026`), PowerShell gate runners.

**Spec:** `docs/superpowers/specs/2026-09-24-translation-unit-pilot-design.md`

## Global Constraints

- Worktree root: `D:\github\Novodex\.claude\worktrees\reconstruction-progress-5b3127` (call it `$WT`). All paths below are relative to it. Build directory: `$WT\build`.
- Evidence root: `docs/reconstruction/novodex-physics` (call it `$EV`).
- Public headers are immutable: nothing under `Physics/include/**` or `Foundation/include/**` changes. `verify_public_headers.py` must pass.
- Product code must not forward to, load, or read the oracle DLL.
- No row is promoted above `reconstructed`. No closure ledger rule changes; ledger counts/reasons change only as the validator requires for newly reconstructed rows.
- No expected transcript line in `$EV/tools/gate_targets.ps1` is edited to make a gate pass. A transcript difference is a defect in the new source.
- The parameterised row models in `Physics/src/ObjectModel.cpp` stay; they may only gain a pointer comment to the product row.
- Every product function in `Physics/src/core/` is preceded by a comment line naming its stable ID, RVA and size, in this exact form: `// phys_fn_004330 (0x000a8d40, 281 B)`. The validator matches on the stable ID.
- Tabs for indentation in C++ (Foundation style). Python follows the existing tools' style (4 spaces, module docstring, `main()` returning an exit code: 0 success, 1 substantive failure, 2 unreadable input).
- Tool tests run with: `PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p 'test_*.py'`
- Every task appends one row to the timing table in `$EV/evidence/unit-pilot-revolute.md` (created in Task 1): task, start/end wall-clock (local ISO time), rows written, bytes written, notes.
- Do not use bare `git stash`. Commit messages end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

---

### Task 1: Work-unit map

**Files:**
- Create: `docs/reconstruction/novodex-physics/tools/work_units.py`
- Create: `docs/reconstruction/novodex-physics/tools/tests/test_work_units.py`
- Create (generated): `docs/reconstruction/novodex-physics/work_units.json`
- Create: `docs/reconstruction/novodex-physics/evidence/unit-pilot-revolute.md`
- Modify: `docs/superpowers/specs/2026-09-24-translation-unit-pilot-design.md` (section 4.1 extent rule wording)

**Interfaces:**
- Consumes: `reconcile_analysis.SOURCE_MARK` (str), `reconcile_analysis.translation_unit_spans(files: dict[int, str]) -> list[tuple[int, int, str]]` (raises `ValueError` on interleaved spans).
- Produces:
  - `work_units.load_edges(dot_text: str) -> set[tuple[str, str]]` — `(caller_id, callee_id)` pairs.
  - `work_units.load_rows(inventory: dict) -> list[dict]` — rows `{id, rva:int, size, kind, state, phase}` sorted by `rva`.
  - `work_units.source_seeds(ghidra: dict, rows: list[dict]) -> dict[int, str]` — owning-row rva → unit name.
  - `work_units.build_units(rows, seeds, edges) -> list[dict]` — unit records (schema below), ordered by first row rva.
  - `work_units.json`: `{"schema_version": 1, "units": [<record>...]}` where a record is `{"unit": str, "evidenced_span": [hex, hex] | null, "inferred_extent": [id...], "ambiguous_rows": [id...], "rows": {state: n}, "bytes": {state: n}, "phases": {"6": n}}`. Gap units are named `gap:<left>..<right>` with `<start>`/`<end>` sentinels; their rows are all in `ambiguous_rows`.

- [ ] **Step 1: Record the start time and create the measurement note**

Create `docs/reconstruction/novodex-physics/evidence/unit-pilot-revolute.md`:

```markdown
# Translation-unit pilot: revolute joint

Design: `docs/superpowers/specs/2026-09-24-translation-unit-pilot-design.md`.
Plan: `docs/superpowers/plans/2026-09-25-translation-unit-pilot.md`.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
```

- [ ] **Step 2: Write the failing tests**

Create `docs/reconstruction/novodex-physics/tools/tests/test_work_units.py`:

```python
import sys
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import work_units  # noqa: E402


def row(n, rva, size=0x10, kind="code", state="discovered", phase=6):
    return {"id": f"phys_fn_{n:06d}", "rva": rva, "size": size, "kind": kind,
            "state": state, "phase": phase}


def fn(n):
    return f"phys_fn_{n:06d}"


class BuildUnitsTest(unittest.TestCase):
    def setUp(self):
        # A.cpp is evidenced over rows 1-2, B.cpp at row 6; rows 3-5 lie between.
        self.rows = [row(1, 0x1000), row(2, 0x1010), row(3, 0x1020), row(4, 0x1030),
                     row(5, 0x1040), row(6, 0x1050),
                     row(7, 0x1060, kind="compiler_artifact", state="classified", phase=8)]
        self.seeds = {0x1000: "A.cpp", 0x1010: "A.cpp", 0x1050: "B.cpp"}

    def unit(self, units, name):
        return next(u for u in units if u["unit"] == name)

    def test_row_with_an_edge_into_the_left_unit_joins_it(self):
        units = work_units.build_units(self.rows, self.seeds, {(fn(3), fn(1))})
        self.assertIn(fn(3), self.unit(units, "A.cpp")["inferred_extent"])

    def test_growth_follows_a_chain_of_edges(self):
        edges = {(fn(3), fn(1)), (fn(4), fn(3))}
        units = work_units.build_units(self.rows, self.seeds, edges)
        self.assertEqual(self.unit(units, "A.cpp")["inferred_extent"],
                         [fn(1), fn(2), fn(3), fn(4)])

    def test_a_row_touching_both_units_stops_growth(self):
        edges = {(fn(3), fn(1)), (fn(3), fn(6))}
        units = work_units.build_units(self.rows, self.seeds, edges)
        self.assertNotIn(fn(3), self.unit(units, "A.cpp")["inferred_extent"])
        self.assertIn(fn(3), self.unit(units, "gap:A.cpp..B.cpp")["ambiguous_rows"])

    def test_the_right_unit_grows_downward(self):
        units = work_units.build_units(self.rows, self.seeds, {(fn(6), fn(5))})
        self.assertEqual(self.unit(units, "B.cpp")["inferred_extent"], [fn(5), fn(6)])

    def test_unconnected_rows_are_ambiguous(self):
        units = work_units.build_units(self.rows, self.seeds, set())
        gap = self.unit(units, "gap:A.cpp..B.cpp")
        self.assertEqual(gap["inferred_extent"], [])
        self.assertEqual(gap["ambiguous_rows"], [fn(3), fn(4), fn(5)])
        self.assertIsNone(gap["evidenced_span"])

    def test_partition_covers_every_code_row_exactly_once(self):
        units = work_units.build_units(self.rows, self.seeds, {(fn(3), fn(1))})
        seen = [i for u in units for i in u["inferred_extent"] + u["ambiguous_rows"]]
        self.assertEqual(sorted(seen), [fn(n) for n in range(1, 7)])

    def test_rows_before_the_first_span_form_a_start_gap(self):
        rows = [row(9, 0x0f00)] + self.rows
        units = work_units.build_units(rows, self.seeds, set())
        self.assertEqual(self.unit(units, "gap:<start>..A.cpp")["ambiguous_rows"], [fn(9)])
        self.assertEqual(units[0]["unit"], "gap:<start>..A.cpp")

    def test_interleaved_spans_are_rejected(self):
        seeds = {0x1000: "A.cpp", 0x1030: "A.cpp", 0x1010: "B.cpp"}
        with self.assertRaises(ValueError):
            work_units.build_units(self.rows, seeds, set())

    def test_counts_are_per_state_bytes_and_phase(self):
        rows = list(self.rows)
        rows[1] = row(2, 0x1010, state="reconstructed")
        units = work_units.build_units(rows, self.seeds, set())
        a = self.unit(units, "A.cpp")
        self.assertEqual(a["rows"], {"discovered": 1, "reconstructed": 1})
        self.assertEqual(a["bytes"], {"discovered": 0x10, "reconstructed": 0x10})
        self.assertEqual(a["phases"], {"6": 2})
        self.assertEqual(a["evidenced_span"], ["0x00001000", "0x00001010"])


class LoadEdgesTest(unittest.TestCase):
    def test_reads_quoted_dot_edges_and_ignores_nodes(self):
        dot = ('digraph g {\n  "phys_fn_000001" [rva="0x00001000",phase=2];\n'
               '  "phys_fn_000001" -> "phys_fn_000002";\n}\n')
        self.assertEqual(work_units.load_edges(dot), {(fn(1), fn(2))})


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 3: Run the tests to verify they fail**

Run: `PYTHONDONTWRITEBYTECODE=1 python -m unittest docs/reconstruction/novodex-physics/tools/tests/test_work_units.py -v`
Expected: `ModuleNotFoundError: No module named 'work_units'`.

- [ ] **Step 4: Write the implementation**

Create `docs/reconstruction/novodex-physics/tools/work_units.py`:

```python
"""Group the census's code rows into the translation units the linker laid out.

The image names 57 translation units through NX_ASSERT __FILE__ strings, and each
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

from reconcile_analysis import SOURCE_MARK, translation_unit_spans  # noqa: E402


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
    files = {}
    for entry in ghidra["strings"]:
        index = entry["value"].find(SOURCE_MARK)
        if index >= 0:
            files[int(entry["rva"], 16)] = entry["value"][index + len(SOURCE_MARK):]
    starts = [r["rva"] for r in rows]
    seeds = {}
    for reference in ghidra["references"]:
        if not reference["to_rva"]:
            continue
        name = files.get(int(reference["to_rva"], 16))
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
```

- [ ] **Step 5: Run the tests to verify they pass**

Run: `PYTHONDONTWRITEBYTECODE=1 python -m unittest docs/reconstruction/novodex-physics/tools/tests/test_work_units.py -v`
Expected: 10 tests, `OK`.

- [ ] **Step 6: Generate `work_units.json` and inspect the pilot units**

Run:
```bash
cd docs/reconstruction/novodex-physics && python tools/work_units.py --inventory inventory.json --ghidra oracle/ghidra/manifest.json --dependencies oracle/dependencies.dot --output work_units.json
```
Expected: exit 0, `units=` line with `named=57`. If `translation_unit_spans` raises because a non-`SOURCE_PHASES` name interleaves, stop and report the message; do not weaken the check.

Then print the three units the pilot uses:
```bash
cd docs/reconstruction/novodex-physics && python -c "
import json
u={x['unit']:x for x in json.load(open('work_units.json'))['units']}
for n in ('core\\\\RevoluteJoint.cpp','core\\\\NpRevoluteJoint.cpp','Joint.cpp'):
    x=u[n]; print(n, x['evidenced_span'], len(x['inferred_extent']), x['inferred_extent'][0], x['inferred_extent'][-1], x['rows'], x['bytes'])"
```
Expected: RevoluteJoint.cpp's extent starts at or before `phys_fn_004328` and ends at or after `phys_fn_004370`. Record the three lines in the Task 1 timing-row notes.

- [ ] **Step 7: Update the spec's extent rule wording**

In `docs/superpowers/specs/2026-09-24-translation-unit-pilot-design.md` section 4.1, replace the paragraph starting `**Extent rule.**` with:

```markdown
**Extent rule.** A named unit's extent is its evidenced span, grown outward one
row at a time while the next row has a direct call edge into the unit's current
rows and none into the neighbouring unit's rows. The left unit grows first, then
the right unit over what remains. Rows neither side claims are the
`ambiguous_rows` of a `gap:<left>..<right>` unit. A row with no edge into either
unit is ambiguous, not assigned: unconnected code (for example the vendored
OPCODE region) must not flow into its neighbour. The rule is descriptive: it
writes nothing into `inventory.json`.
```

- [ ] **Step 8: Run the full tool suite, append the timing row, commit**

Run: `PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p 'test_*.py'`
Expected: `OK` (all existing tests plus 10 new).

```bash
git add docs/reconstruction/novodex-physics/tools/work_units.py docs/reconstruction/novodex-physics/tools/tests/test_work_units.py docs/reconstruction/novodex-physics/work_units.json docs/reconstruction/novodex-physics/evidence/unit-pilot-revolute.md docs/superpowers/specs/2026-09-24-translation-unit-pilot-design.md
git commit -m "Map census code rows to translation-unit work units

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 2: Pinned Ghidra decompile supplement

**Files:**
- Create: `docs/reconstruction/novodex-physics/ghidra/DecompileSupplement.java`
- Create (generated): `docs/reconstruction/novodex-physics/oracle/ghidra/supplement.json`

**Interfaces:**
- Consumes: `work_units.json` (Task 1) to choose the requested rows; the existing Ghidra project `D:\FlamingEnt__\novodex-analysis\novodex-physics\PhysicsOracle` (already analysed and typed).
- Produces: `supplement.json`:
  `{"schema_version": 1, "ghidra_version": str, "analysis_options_sha256": str, "decompiler_timeout_seconds": 60, "decompiler_simplification_style": "decompile", "requested": [hex...], "functions": [{"rva": hex, "status": "ok"|"create_failed"|"decompile_failed", "created": bool, "prototype": str|null, "calling_convention": str|null, "stack_purge": int|null, "body": [[hex, hex]...], "decompiler_c": str|null, "error": str|null}]}`. RVAs are `0x%08x` strings.

- [ ] **Step 1: Write the script**

Create `docs/reconstruction/novodex-physics/ghidra/DecompileSupplement.java`:

```java
/* Decompile census rows Ghidra never made functions, without changing the project.
 *
 * Run with -readOnly -noanalysis against the analysed PhysicsOracle project, so a
 * function created here exists only for this session. Decompiler settings match
 * ExportPhysicsAnalysis.java, so the output is comparable to the committed manifest.
 *
 * Usage: -postScript DecompileSupplement.java <output.json> <analysis_options_sha256> <rva>...
 */
//@category Novodex

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.framework.Application;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressRange;
import ghidra.program.model.listing.Function;

public class DecompileSupplement extends GhidraScript {

	private static final int DECOMPILE_TIMEOUT_SECONDS = 60;
	private static final String SIMPLIFICATION_STYLE = "decompile";

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length < 3) {
			throw new IllegalArgumentException(
				"usage: <output.json> <analysis_options_sha256> <rva>...");
		}
		Address base = currentProgram.getImageBase();
		DecompInterface decompiler = new DecompInterface();
		decompiler.setOptions(new DecompileOptions());
		decompiler.toggleCCode(true);
		decompiler.toggleSyntaxTree(false);
		decompiler.setSimplificationStyle(SIMPLIFICATION_STYLE);
		if (!decompiler.openProgram(currentProgram)) {
			throw new IllegalStateException(
				"the decompiler refused this program: " + decompiler.getLastMessage());
		}
		StringBuilder json = new StringBuilder();
		json.append("{\n \"schema_version\": 1,\n");
		json.append(" \"ghidra_version\": ").append(q(Application.getApplicationVersion())).append(",\n");
		json.append(" \"analysis_options_sha256\": ").append(q(args[1])).append(",\n");
		json.append(" \"decompiler_timeout_seconds\": ").append(DECOMPILE_TIMEOUT_SECONDS).append(",\n");
		json.append(" \"decompiler_simplification_style\": ").append(q(SIMPLIFICATION_STYLE)).append(",\n");
		json.append(" \"requested\": [");
		for (int i = 2; i < args.length; i++) {
			json.append(i > 2 ? ", " : "").append(q(hex(Long.decode(args[i]))));
		}
		json.append("],\n \"functions\": [\n");
		try {
			for (int i = 2; i < args.length; i++) {
				long rva = Long.decode(args[i]);
				json.append(record(rva, base.add(rva), decompiler));
				json.append(i + 1 < args.length ? ",\n" : "\n");
			}
		}
		finally {
			decompiler.dispose();
		}
		json.append(" ]\n}\n");
		Files.write(Paths.get(args[0]), json.toString().getBytes(StandardCharsets.UTF_8));
	}

	private String record(long rva, Address entry, DecompInterface decompiler) {
		Function function = getFunctionAt(entry);
		boolean created = false;
		if (function == null) {
			function = createFunction(entry, null);
			created = function != null;
		}
		StringBuilder r = new StringBuilder();
		r.append("  {\"rva\": ").append(q(hex(rva))).append(", \"created\": ").append(created);
		if (function == null) {
			r.append(", \"status\": \"create_failed\", \"prototype\": null, \"calling_convention\": null,")
				.append(" \"stack_purge\": null, \"body\": [], \"decompiler_c\": null,")
				.append(" \"error\": ").append(q("createFunction returned null")).append("}");
			return r.toString();
		}
		long imageBase = currentProgram.getImageBase().getOffset();
		r.append(", \"prototype\": ").append(q(function.getSignature().getPrototypeString()));
		r.append(", \"calling_convention\": ").append(q(function.getCallingConventionName()));
		r.append(", \"stack_purge\": ").append(function.getStackPurgeSize());
		r.append(", \"body\": [");
		boolean first = true;
		for (AddressRange range : function.getBody()) {
			r.append(first ? "" : ", ").append("[")
				.append(q(hex(range.getMinAddress().getOffset() - imageBase))).append(", ")
				.append(q(hex(range.getMaxAddress().getOffset() - imageBase + 1))).append("]");
			first = false;
		}
		r.append("]");
		DecompileResults results =
			decompiler.decompileFunction(function, DECOMPILE_TIMEOUT_SECONDS, monitor);
		if (results != null && results.decompileCompleted()
				&& results.getDecompiledFunction() != null) {
			r.append(", \"status\": \"ok\", \"decompiler_c\": ")
				.append(q(results.getDecompiledFunction().getC())).append(", \"error\": null}");
		}
		else {
			String error = results == null ? "no results" : results.getErrorMessage();
			r.append(", \"status\": \"decompile_failed\", \"decompiler_c\": null, \"error\": ")
				.append(q(error)).append("}");
		}
		return r.toString();
	}

	private static String hex(long value) {
		return String.format("0x%08x", value);
	}

	private static String q(String s) {
		if (s == null) {
			return "null";
		}
		StringBuilder out = new StringBuilder("\"");
		for (char c : s.toCharArray()) {
			switch (c) {
				case '"': out.append("\\\""); break;
				case '\\': out.append("\\\\"); break;
				case '\n': out.append("\\n"); break;
				case '\r': out.append("\\r"); break;
				case '\t': out.append("\\t"); break;
				default:
					if (c < 0x20) {
						out.append(String.format("\\u%04x", (int) c));
					}
					else {
						out.append(c);
					}
			}
		}
		return out.append('"').toString();
	}
}
```

- [ ] **Step 2: Choose the requested RVAs**

Every code row in the RevoluteJoint.cpp, NpRevoluteJoint.cpp and Joint.cpp extents, plus the base rows named in the spec, that has no `ok` decompile in the manifest:

```bash
cd docs/reconstruction/novodex-physics && python -c "
import json
g={int(f['rva'],16):f for f in json.load(open('oracle/ghidra/manifest.json'))['functions']}
inv={r['id']:r for r in json.load(open('inventory.json'))['functions']}
u={x['unit']:x for x in json.load(open('work_units.json'))['units']}
ids=set()
for n in ('core\\\\RevoluteJoint.cpp','core\\\\NpRevoluteJoint.cpp','Joint.cpp'): ids|=set(u[n]['inferred_extent'])
ids|={'phys_fn_%06d'%k for k in (4064,4066,4093,4097,4123,4127,4129,4135,4389,4391,4393,4372,4374,4721,4723,4725)}
out=sorted(int(inv[i]['rva'],16) for i in ids if not (g.get(int(inv[i]['rva'],16)) or {}).get('decompiler_status')=='ok')
import os; text=' '.join('0x%x'%a for a in out); open(os.path.join(os.environ['TEMP'],'supplement_rvas.txt'),'w').write(text); print(text)"
```
(Python writes the list to the Windows `%TEMP%`, which is where the PowerShell step reads it.)
Expected: a space-separated list that includes `0xa9650 0xaa060 0xab840`.

- [ ] **Step 3: Verify the toolchain pin and run the script**

The Ghidra project must not be open in a Ghidra GUI (a lock error means it is; close it, do not copy the project). Run from `$WT` in PowerShell:

```powershell
$ev='docs\reconstruction\novodex-physics'
python "$ev\tools\verify_toolchain.py" --pin "$ev\analysis_toolchain.json" --ghidra-home D:\ghidra
$hash=(Get-Content "$ev\analysis_toolchain.json" -Raw | ConvertFrom-Json).analysis_options_sha256
$rvas=(Get-Content "$env:TEMP\supplement_rvas.txt" -Raw).Trim().Split(' ')
& D:\ghidra\support\analyzeHeadless.bat D:\FlamingEnt__\novodex-analysis\novodex-physics PhysicsOracle -process NxPhysics.dll -readOnly -noanalysis -scriptPath "$ev\ghidra" -postScript DecompileSupplement.java "$PWD\$ev\oracle\ghidra\supplement.json" $hash @rvas
```
Expected: toolchain verification exits 0; headless log ends with the script completing and no `ERROR REPORT` for the script; `supplement.json` exists.

- [ ] **Step 4: Check the output**

```bash
cd docs/reconstruction/novodex-physics && python -c "
import json;s=json.load(open('oracle/ghidra/supplement.json'))
from collections import Counter;print(s['ghidra_version'],Counter(f['status'] for f in s['functions']))
for f in s['functions']:
    if f['status']!='ok': print(f['rva'],f['status'],f['error'])
print({f['rva']:len(f['decompiler_c'] or '') for f in s['functions'] if f['rva'] in ('0x000a9650','0x000aa060','0x000ab840')})"
```
Expected: `12.1.2`, and all three large rows `ok` with non-empty C. Any `create_failed`/`decompile_failed` row is listed in the timing-row notes; those rows fall back to disassembly in the bundle.

Run it a second time with the same command and confirm `git diff --stat` shows no change to `supplement.json` (determinism). If it differs, record the differing fields in the notes.

- [ ] **Step 5: Append the timing row and commit**

```bash
git add docs/reconstruction/novodex-physics/ghidra/DecompileSupplement.java docs/reconstruction/novodex-physics/oracle/ghidra/supplement.json docs/reconstruction/novodex-physics/evidence/unit-pilot-revolute.md
git commit -m "Decompile pilot rows Ghidra never made functions

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 3: Unit reference bundles

**Files:**
- Create: `docs/reconstruction/novodex-physics/tools/unit_bundle.py`
- Create: `docs/reconstruction/novodex-physics/tools/tests/test_unit_bundle.py`
- Create (generated): `docs/reconstruction/novodex-physics/units/core__RevoluteJoint.cpp.md`, `units/core__NpRevoluteJoint.cpp.md`, `units/Joint.cpp.md`

**Interfaces:**
- Consumes: `work_units.load_edges`; `work_units.json` (Task 1); `oracle/ghidra/supplement.json` (Task 2, optional input).
- Produces:
  - `unit_bundle.unit_filename(unit: str) -> str`
  - `unit_bundle.decompile_source(rva: int, size: int, oracles) -> tuple[str, str]` — label is one of `"ghidra manifest"`, `"ghidra supplement"`, `"capstone disassembly"`.
  - `unit_bundle.Oracles(inventory, ghidra, capstone, supplement, edges)` with attributes `ghidra`, `supplement` (dicts keyed by int rva) and method `instructions(lo, hi)`.
  - `unit_bundle.render_unit(unit: dict, unit_of: dict[str, str], oracles) -> str`

- [ ] **Step 1: Write the failing tests**

Create `docs/reconstruction/novodex-physics/tools/tests/test_unit_bundle.py`:

```python
import sys
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import unit_bundle  # noqa: E402


def ins(rva, mnemonic, operands="", indirect=False):
    return {"rva": f"0x{rva:08x}", "size": 1, "bytes": "90", "mnemonic": mnemonic,
            "operands": operands, "flow": "call" if indirect else "sequential",
            "indirect": indirect, "target_rva": None, "stack_delta": 0}


def oracles(ghidra_functions=(), supplement_functions=(), data_objects=(), edges=()):
    inventory = {"functions": [
        {"id": "phys_fn_000001", "rva": "0x00001000", "size": 3, "kind": "code",
         "state": "discovered", "phase": 6, "source": None},
        {"id": "phys_fn_000002", "rva": "0x00001010", "size": 2, "kind": "code",
         "state": "reconstructed", "phase": 6, "source": "x"}],
        "data_objects": list(data_objects)}
    ghidra = {"functions": list(ghidra_functions),
              "strings": [{"rva": "0x00002000", "value": "Joint.cpp"}],
              "references": [{"from_rva": "0x00001001", "to_rva": "0x00002000"}]}
    capstone = {"instructions": [ins(0x1000, "push", "ebp"),
                                 ins(0x1001, "call", "dword ptr [eax + 0x14]", True),
                                 ins(0x1002, "ret"), ins(0x1010, "ret")]}
    return unit_bundle.Oracles(inventory, ghidra, capstone,
                               {"functions": list(supplement_functions)}, set(edges))


class DecompileSourceTest(unittest.TestCase):
    def test_manifest_decompile_wins(self):
        o = oracles(ghidra_functions=[{"rva": "0x00001000", "decompiler_status": "ok",
                                       "decompiler_c": "void f(void) {}"}],
                    supplement_functions=[{"rva": "0x00001000", "status": "ok",
                                           "decompiler_c": "other"}])
        self.assertEqual(unit_bundle.decompile_source(0x1000, 3, o),
                         ("ghidra manifest", "void f(void) {}"))

    def test_supplement_used_when_manifest_has_none(self):
        o = oracles(supplement_functions=[{"rva": "0x00001000", "status": "ok",
                                           "decompiler_c": "void g(void) {}"}])
        self.assertEqual(unit_bundle.decompile_source(0x1000, 3, o),
                         ("ghidra supplement", "void g(void) {}"))

    def test_failed_supplement_falls_back_to_disassembly(self):
        o = oracles(supplement_functions=[{"rva": "0x00001000", "status": "decompile_failed",
                                           "decompiler_c": None}])
        label, text = unit_bundle.decompile_source(0x1000, 3, o)
        self.assertEqual(label, "capstone disassembly")
        self.assertIn("0x00001000  push ebp", text)
        self.assertNotIn("0x00001010", text)


class RenderUnitTest(unittest.TestCase):
    def test_bundle_names_rows_edges_indirect_calls_strings_and_dispatch(self):
        o = oracles(data_objects=[{"id": "phys_data_000009", "rva": "0x00003000",
                                   "notes": '{"slots":3,"targets":["0x0000f000","0x0000f004","0x00001010"]}'}],
                    edges={("phys_fn_000001", "phys_fn_000002"),
                           ("phys_fn_000001", "phys_fn_000077")})
        unit = {"unit": "core\\A.cpp", "evidenced_span": ["0x00001000", "0x00001000"],
                "inferred_extent": ["phys_fn_000001", "phys_fn_000002"], "ambiguous_rows": [],
                "rows": {}, "bytes": {}, "phases": {}}
        text = unit_bundle.render_unit(unit, {"phys_fn_000077": "Other.cpp"}, o)
        self.assertIn("# core\\A.cpp", text)
        self.assertIn("## phys_fn_000001 (0x00001000, 3 B, discovered)", text)
        self.assertIn("phys_data_000009 slot 2 -> phys_fn_000002", text)
        self.assertIn("0x00001001  call dword ptr [eax + 0x14]", text)
        self.assertIn("Joint.cpp", text)
        self.assertIn("Other.cpp: phys_fn_000077", text)

    def test_unit_filename_is_flat_and_safe(self):
        self.assertEqual(unit_bundle.unit_filename("core\\RevoluteJoint.cpp"),
                         "core__RevoluteJoint.cpp.md")
        self.assertEqual(unit_bundle.unit_filename("gap:A.cpp..B.cpp"),
                         "gap__A.cpp__to__B.cpp.md")


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `PYTHONDONTWRITEBYTECODE=1 python -m unittest docs/reconstruction/novodex-physics/tools/tests/test_unit_bundle.py -v`
Expected: `ModuleNotFoundError: No module named 'unit_bundle'`.

- [ ] **Step 3: Write the implementation**

Create `docs/reconstruction/novodex-physics/tools/unit_bundle.py`:

```python
"""Write one reference document per work unit, from committed oracle data.

A bundle is what a writer reconstructing a whole translation unit reads: every row
in address order with its state, prototype, direct edges, indirect-call sites,
referenced strings and decompile, plus the dispatch tables that name the unit's
rows and the unit's dependencies grouped by owning unit. The decompile comes from
the committed Ghidra manifest, else from the pinned supplement, else from Capstone
disassembly, and the bundle says which.

Usage: python unit_bundle.py --work-units work_units.json --inventory inventory.json
           --ghidra oracle/ghidra/manifest.json --capstone oracle/capstone/manifest.json
           --dependencies oracle/dependencies.dot [--supplement oracle/ghidra/supplement.json]
           --unit "core\\RevoluteJoint.cpp" [--unit ...] --output-dir units
"""

import argparse
import bisect
import json
import re
import sys
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parent))

from work_units import load_edges  # noqa: E402


def unit_filename(unit):
    return re.sub(r"[\\/:]+", "__", unit).replace("..", "__to__") + ".md"


class Oracles:
    def __init__(self, inventory, ghidra, capstone, supplement, edges):
        self.rows = {r["id"]: r for r in inventory["functions"]}
        self.by_rva = {int(r["rva"], 16): r["id"] for r in inventory["functions"]}
        self.ghidra = {int(f["rva"], 16): f for f in ghidra["functions"]}
        self.supplement = {int(f["rva"], 16): f for f in (supplement or {}).get("functions", [])}
        self.ins = sorted(capstone["instructions"], key=lambda i: int(i["rva"], 16))
        self.ins_rva = [int(i["rva"], 16) for i in self.ins]
        strings = {int(s["rva"], 16): s["value"] for s in ghidra["strings"]}
        self.refs = sorted((int(r["from_rva"], 16), strings[int(r["to_rva"], 16)])
                           for r in ghidra["references"]
                           if r["to_rva"] and int(r["to_rva"], 16) in strings)
        self.ref_rva = [a for a, _ in self.refs]
        self.callers, self.callees = {}, {}
        for a, b in edges:
            self.callees.setdefault(a, set()).add(b)
            self.callers.setdefault(b, set()).add(a)
        self.dispatch = []
        for obj in inventory.get("data_objects", []):
            try:
                note = json.loads(obj.get("notes") or "")
            except ValueError:
                continue
            if isinstance(note, dict) and isinstance(note.get("targets"), list):
                for slot, target in enumerate(note["targets"]):
                    self.dispatch.append((obj["id"], obj["rva"], slot, int(target, 16)))

    def instructions(self, lo, hi):
        i = bisect.bisect_left(self.ins_rva, lo)
        out = []
        while i < len(self.ins) and self.ins_rva[i] < hi:
            out.append(self.ins[i])
            i += 1
        return out

    def strings(self, lo, hi):
        i = bisect.bisect_left(self.ref_rva, lo)
        out = []
        while i < len(self.refs) and self.ref_rva[i] < hi:
            out.append(self.refs[i])
            i += 1
        return out


def decompile_source(rva, size, oracles):
    function = oracles.ghidra.get(rva)
    if function and function.get("decompiler_status") == "ok" and function.get("decompiler_c"):
        return "ghidra manifest", function["decompiler_c"]
    extra = oracles.supplement.get(rva)
    if extra and extra.get("status") == "ok" and extra.get("decompiler_c"):
        return "ghidra supplement", extra["decompiler_c"]
    lines = [f"{i['rva']}  {i['mnemonic']} {i['operands']}".rstrip()
             for i in oracles.instructions(rva, rva + size)]
    return "capstone disassembly", "\n".join(lines)


def _name(row_id, oracles):
    row = oracles.rows.get(row_id)
    return f"{row_id} ({row['rva']})" if row else row_id


def render_unit(unit, unit_of, oracles):
    ids = sorted(unit["inferred_extent"] + unit["ambiguous_rows"],
                 key=lambda i: int(oracles.rows[i]["rva"], 16))
    own = set(ids)
    out = [f"# {unit['unit']}", "",
           f"Evidenced span: {unit['evidenced_span']}. Rows: {len(ids)} "
           f"({len(unit['ambiguous_rows'])} ambiguous). Generated by tools/unit_bundle.py; "
           f"do not edit by hand.", "", "## Dispatch tables naming this unit's rows", ""]
    rvas = {int(oracles.rows[i]["rva"], 16): i for i in ids}
    tables = [(t, trva, slot, rvas[target]) for t, trva, slot, target in oracles.dispatch
              if target in rvas]
    out += [f"- {t} ({trva}) slot {slot} -> {row}" for t, trva, slot, row in tables] or ["- none"]
    out += ["", "## External dependencies by unit", ""]
    external = {}
    for i in ids:
        for callee in oracles.callees.get(i, ()):
            if callee not in own:
                external.setdefault(unit_of.get(callee, "unassigned"), set()).add(callee)
    out += [f"- {u}: {', '.join(sorted(c))}" for u, c in sorted(external.items())] or ["- none"]
    for i in ids:
        row = oracles.rows[i]
        rva, size = int(row["rva"], 16), row["size"]
        function = oracles.ghidra.get(rva) or oracles.supplement.get(rva) or {}
        label, text = decompile_source(rva, size, oracles)
        indirect = [f"{x['rva']}  {x['mnemonic']} {x['operands']}"
                    for x in oracles.instructions(rva, rva + size) if x["indirect"]]
        strings = [f"{hex(a)}: {s}" for a, s in oracles.strings(rva, rva + size)]
        out += ["", f"## {i} ({row['rva']}, {size} B, {row['state']})", "",
                f"- ambiguous: {'yes' if i in unit['ambiguous_rows'] else 'no'}",
                f"- source: {row.get('source')}",
                f"- implementation: {row.get('implementation')}",
                f"- prototype: {function.get('prototype')}",
                f"- calling convention: {function.get('calling_convention')}, "
                f"stack purge: {function.get('stack_purge')}",
                f"- callers: {', '.join(_name(c, oracles) for c in sorted(oracles.callers.get(i, ()))) or 'none'}",
                f"- callees: {', '.join(_name(c, oracles) for c in sorted(oracles.callees.get(i, ()))) or 'none'}",
                f"- indirect calls: {'; '.join(indirect) or 'none'}",
                f"- strings: {'; '.join(strings) or 'none'}",
                "", f"Decompile ({label}):", "", "```c" if label != "capstone disassembly" else "```asm",
                text, "```"]
    return "\n".join(out) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    for name in ("--work-units", "--inventory", "--ghidra", "--capstone", "--dependencies",
                 "--output-dir"):
        parser.add_argument(name, required=True)
    parser.add_argument("--supplement")
    parser.add_argument("--unit", action="append", required=True)
    args = parser.parse_args(argv)
    try:
        load = lambda p: json.loads(Path(p).read_text(encoding="utf-8"))  # noqa: E731
        units = {u["unit"]: u for u in load(args.work_units)["units"]}
        oracles = Oracles(load(args.inventory), load(args.ghidra), load(args.capstone),
                          load(args.supplement) if args.supplement else None,
                          load_edges(Path(args.dependencies).read_text(encoding="utf-8")))
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    unit_of = {i: name for name, u in units.items()
               for i in u["inferred_extent"] + u["ambiguous_rows"]}
    output = Path(args.output_dir)
    output.mkdir(parents=True, exist_ok=True)
    for name in args.unit:
        if name not in units:
            print(f"error: no unit named {name!r}", file=sys.stderr)
            return 1
        path = output / unit_filename(name)
        path.write_text(render_unit(units[name], unit_of, oracles), encoding="utf-8",
                        newline="\n")
        print(f"wrote {path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `PYTHONDONTWRITEBYTECODE=1 python -m unittest docs/reconstruction/novodex-physics/tools/tests/test_unit_bundle.py -v`
Expected: 5 tests, `OK`.

- [ ] **Step 5: Generate the three pilot bundles**

```bash
cd docs/reconstruction/novodex-physics && python tools/unit_bundle.py --work-units work_units.json --inventory inventory.json --ghidra oracle/ghidra/manifest.json --capstone oracle/capstone/manifest.json --dependencies oracle/dependencies.dot --supplement oracle/ghidra/supplement.json --unit "core\\RevoluteJoint.cpp" --unit "core\\NpRevoluteJoint.cpp" --unit "Joint.cpp" --output-dir units
```
Expected: three `wrote units/...` lines. Check that `units/core__RevoluteJoint.cpp.md` shows `Decompile (ghidra supplement)` under `phys_fn_004360` and that the dispatch section names `phys_data_002684`.

- [ ] **Step 6: Run the full tool suite, append the timing row, commit**

Run: `PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p 'test_*.py'`
Expected: `OK`.

```bash
git add docs/reconstruction/novodex-physics/tools/unit_bundle.py docs/reconstruction/novodex-physics/tools/tests/test_unit_bundle.py docs/reconstruction/novodex-physics/units docs/reconstruction/novodex-physics/evidence/unit-pilot-revolute.md
git commit -m "Generate per-unit reconstruction bundles for the revolute pilot

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 4: Pilot contract recovery

This task writes no product code. It establishes, from the bundles and the oracle, the facts every later task needs, and records them in one document. Nothing in Tasks 5–10 may contradict it without first amending it.

**Files:**
- Create: `docs/reconstruction/novodex-physics/units/revolute-contract.md`

**Interfaces:**
- Consumes: the three bundles (Task 3); `Physics/src/Scene.cpp` (`NxSceneInternal::createJoint`, lines around 1392–1500); `Physics/src/include/NpJoint.h`; `Physics/src/NpJoint.cpp`; `$EV/evidence/phase6-joints.md`; `$EV/object_model.json`.
- Produces: `revolute-contract.md` with these exact section headings, which later tasks cite: `## Row assignment`, `## Construction chain`, `## Object layouts`, `## Dispatch tables`, `## Dependency closure`, `## Existing candidate code`, `## Task split`.

- [ ] **Step 1: Row assignment**

Check each unit's inferred extent by hand against the dispatch tables and call edges in the bundles. Specifically decide, with the evidence line for each: whether `phys_fn_004372` (`0xac700`) and `phys_fn_004374` (`0xad0b0`) belong to RevoluteJoint.cpp; whether `phys_fn_004675/4677/4679` (`0xb2c80`–`0xb2cd0`) and `phys_fn_004721/4723/4725` (`0xb3340`–`0xb33a0`) belong to NpRevoluteJoint.cpp; which of `004064 004066 004093 004097 004123 004127 004129 004135 004389 004391 004393` belong to Joint.cpp per `work_units.json` and which to another unit. Write a table: stable ID, RVA, size, state, assigned file (`core/RevoluteJoint.cpp`, `core/NpRevoluteJoint.cpp`, `core/Joint.cpp`, or `core/JointSupport.cpp` for pulled-in rows outside those three units), evidence.

- [ ] **Step 2: Construction chain**

From the `0x142c0` (`phys_fn_000665`) disassembly in the Capstone manifest, trace the revolute switch case: every allocation size, every constructor call with its target row, and what `createJoint` returns (the public object pointer). Reconcile with the existing comment in `Scene.cpp` that names `0xad6e0` and with `004366`'s caller edge. Record the chain as an ordered list of `row → callee row (purpose)`.

- [ ] **Step 3: Object layouts**

Record, with offsets, sizes and the row that establishes each field: the public `NpRevoluteJoint` object (the existing `NpJointObject` says 0x17c), the internal revolute joint object (fields up to at least `+0x1b4`, from 004328/004342/004346/004350), and the Joint base part. Mark any field whose meaning is unknown as `unknown` with the rows that touch it — do not guess names.

- [ ] **Step 4: Dispatch tables**

For `phys_data_002684` and every other table the bundles list for these units: slot → target row → what it is (for public tables, map to the `NxRevoluteJoint`/`NxJoint` virtual in `Physics/include/NxRevoluteJoint.h` and `NxJoint.h` declaration order). Identify which table is the public `NxRevoluteJoint` vtable installed on the object `createJoint` returns.

- [ ] **Step 5: Dependency closure, existing code, task split**

- `## Dependency closure`: every external callee of the pilot rows (from the bundles' "External dependencies" sections), each marked `write` (goes into `core/Joint.cpp`/`core/JointSupport.cpp` in Task 6) or `reuse` (already implemented in the candidate — name the symbol and file, found by grepping `Physics/src` for the stable ID) or `defer` (called only on paths the joint transcript cannot reach; name the path).
- `## Existing candidate code`: every candidate function the new code replaces or must stay compatible with (`nxJointConstruct`, `nxJointDestroy`, `nxJointSizeForType`, `NpJointVtable`, `nxSceneAddJoint`, `NpJointObject`), with file:line.
- `## Task split`: the exact row list for Tasks 6, 7, 8 and 9. Task 7 takes RevoluteJoint.cpp rows under 700 B; Task 8 takes RevoluteJoint.cpp rows of 700 B or more; Task 9 takes NpRevoluteJoint.cpp rows; Task 6 takes Joint.cpp and JointSupport.cpp rows marked `write`.

- [ ] **Step 6: Append the timing row and commit**

```bash
git add docs/reconstruction/novodex-physics/units/revolute-contract.md docs/reconstruction/novodex-physics/evidence/unit-pilot-revolute.md
git commit -m "Record the revolute pilot's recovered contract

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 5: Source scaffold

**Files:**
- Modify: `CMakeLists.txt:67` (`PHYSICS_SOURCES` glob)
- Modify: `CMakeLists.txt:59` (`PHYSICS_HEADERS` glob, only if `Physics/src/include/*.h` does not already pick up `Physics/src/include/core/*.h` — `GLOB_RECURSE` does, so verify and leave it if so)
- Create: `Physics/src/include/core/RevoluteJoint.h`, `Physics/src/include/core/NpRevoluteJoint.h`, `Physics/src/include/core/Joint.h`
- Create: `Physics/src/core/RevoluteJoint.cpp`, `Physics/src/core/NpRevoluteJoint.cpp`, `Physics/src/core/Joint.cpp` (and `Physics/src/core/JointSupport.cpp` if the contract assigns rows to it)

**Interfaces:**
- Consumes: `revolute-contract.md` sections `## Object layouts`, `## Dispatch tables`, `## Task split`.
- Produces: the class and struct declarations Tasks 6–9 fill in. Required names:
  - `class NpRevoluteJoint : public NxRevoluteJoint` in `NpRevoluteJoint.h` — the public object, declaring every `NxRevoluteJoint`/`NxJoint` pure virtual in header order.
  - `class RevoluteJoint` (internal) in `RevoluteJoint.h` and `class Joint` (internal base) in `Joint.h`, with fields at the offsets the contract records, each field commented with its offset. Use a `static_assert(sizeof(...) == 0x...)` for every size the contract establishes.
  - One member function declaration per assigned row, named from the contract (or `rowXXXX` with the stable-ID comment when the contract has no name), with the calling convention the bundle records.

- [ ] **Step 1: Add the core sources to the build**

Edit `CMakeLists.txt` line 67 to:

```cmake
file(GLOB PHYSICS_SOURCES Physics/src/*.cpp Physics/src/core/*.cpp Physics/src/opcode/*.cpp)
```

- [ ] **Step 2: Write the headers and empty-bodied sources**

Declare the classes per the contract. Each `.cpp` starts with the NovodeX banner used in `Physics/src/NpJoint.cpp`, includes its header, and defines each assigned row as a function whose body is the single statement `NX_ASSERT(0);` followed by the type's zero return, preceded by the stable-ID comment line from Global Constraints and the line `// (unimplemented)`. These stubs are not reachable yet: nothing constructs the new classes until Task 10.

- [ ] **Step 3: Configure and build**

Run from `$WT`:
```bash
cmake -S . -B build -A Win32 && cmake --build build --config Release --target NxPhysics
```
Expected: build succeeds with no new warnings in `Physics/src/core/`. Every `static_assert` holds.

- [ ] **Step 4: Run the Phase 6 gate and the validator**

```bash
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 6 -RepoRoot "$PWD" -BuildRoot "$PWD/build"
python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json
```
Expected: gate prints `status=pass`; validator exits 0. (The transcript is unaffected because nothing uses the new classes yet.)

- [ ] **Step 5: Append the timing row and commit**

```bash
git add CMakeLists.txt Physics/src/core Physics/src/include/core docs/reconstruction/novodex-physics/evidence/unit-pilot-revolute.md
git commit -m "Scaffold revolute joint translation units

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tasks 6–9: Write the rows

Tasks 6, 7, 8 and 9 share one procedure and differ only in their row list, which comes from `revolute-contract.md` `## Task split`:

- **Task 6:** `core/Joint.cpp` and `core/JointSupport.cpp` rows marked `write`.
- **Task 7:** `core/RevoluteJoint.cpp` rows under 700 B.
- **Task 8:** `core/RevoluteJoint.cpp` rows of 700 B or more (x87-heavy).
- **Task 9:** `core/NpRevoluteJoint.cpp` rows.

**Files (per task):** Modify only that task's `.cpp` files and, if a declaration must change, the matching header under `Physics/src/include/core/`. A layout or declaration change must also be made in `revolute-contract.md` in the same commit.

**Interfaces:**
- Consumes: the bundle for the unit; `revolute-contract.md`; the declarations from Task 5 and the rows written by earlier tasks.
- Produces: real bodies for the task's rows; nothing else.

**Procedure — for each row in the task's list, in address order:**

- [ ] **Step 1: Read the row's bundle entry**

Read the decompile, the callers/callees, the indirect-call sites and the strings. When the label is `capstone disassembly`, work from the listing. When the decompile disagrees with the listing, the listing wins; record the disagreement in the row's comment.

- [ ] **Step 2: Write the body**

Replace `NX_ASSERT(0);` and `// (unimplemented)` with the recovered behaviour. Rules:
- Field access goes through the named members the contract defines; no raw `this + 0x...` arithmetic where a member exists.
- Calls to other pilot rows call their C++ functions. Calls to `reuse` dependencies call the named existing symbol. Calls to `defer` dependencies keep the call through a declared function that does `NX_ASSERT(0)`, with a comment naming the row and the reason.
- Indirect calls resolve through the vtable the contract names for that receiver, not a guessed slot.
- Assert reports keep the oracle's file/line/expression when the bundle shows them.
- Floating-point: keep the operation order the listing shows; do not algebraically simplify. Use `float` where the listing uses single-precision loads/stores.
- If the parameterised model in `ObjectModel.cpp` already covers this row, add to that model's doc comment: `Product row: Physics/src/core/<File>.cpp.`

- [ ] **Step 3: Build after every row of 500 B or more, and at the end of the task**

Run: `cmake --build build --config Release --target NxPhysics`
Expected: success, no new warnings in `Physics/src/core/`.

- [ ] **Step 4: Regression check at the end of the task**

```bash
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 6 -RepoRoot "$PWD" -BuildRoot "$PWD/build"
```
Expected: `status=pass`. (Before Task 10 this proves only that the build is unaffected.)

- [ ] **Step 5: Append the timing row and commit**

Timing-row notes list: rows written, rows left `defer`, and every decompile/listing disagreement.

```bash
git add Physics/src/core Physics/src/include/core Physics/src/ObjectModel.cpp docs/reconstruction/novodex-physics/units/revolute-contract.md docs/reconstruction/novodex-physics/evidence/unit-pilot-revolute.md
git commit -m "Reconstruct <unit> rows for the revolute pilot (Task N)

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```
(Replace `<unit>` and `N` with the task's unit name and number.)

---

### Task 10: Wire revolute creation to the new code

**Files:**
- Modify: `Physics/src/Scene.cpp` (`NxSceneInternal::createJoint`, the `case 0` revolute branch)
- Modify: `Physics/src/NpJoint.cpp` / `Physics/src/include/NpJoint.h` only as the contract's `## Existing candidate code` requires for non-revolute types to keep working
- Modify: `docs/reconstruction/novodex-physics/units/revolute-contract.md` (record the final wiring)

**Interfaces:**
- Consumes: the construction chain from the contract; the classes from Tasks 5–9.
- Produces: `createJoint` returns an `NpRevoluteJoint*` (as `NxJoint*`) for `NX_JOINT_REVOLUTE`; all other types unchanged.

- [ ] **Step 1: Wire the revolute branch**

In the `case 0` branch of `NxSceneInternal::createJoint`, replace the generic `nxJointConstruct` call with the construction chain the contract records (allocation sizes and constructor calls in oracle order). Registration (`nxSceneAddJoint`) and the marker logic stay as they are unless the contract shows the revolute chain differs.

- [ ] **Step 2: Build and run the joint gate**

```bash
cmake --build build --config Release --target NxPhysics
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 6 -RepoRoot "$PWD" -BuildRoot "$PWD/build"
```
Expected: `status=pass`, meaning the `NxPhysicsJointTests` and `NxPhysicsJointStagedPairTests` transcripts are byte-identical to the oracle's.

- [ ] **Step 3: If the transcript differs, debug with superpowers:systematic-debugging**

Treat every differing line as a defect in the new source. Find the first differing line, trace which row produced it, fix that row, rebuild, and re-run. Do not edit any expected line in `gate_targets.ps1`. Record each defect (line, row, cause, fix) in the timing-row notes.

If a difference cannot be resolved within this task, revert only Step 1's wiring so the revolute branch uses the generic path again, confirm the gate passes, and record the unresolved difference and the rows involved in the notes. Those rows stay `discovered` in Task 11.

- [ ] **Step 4: Run gates 2, 3, 4, 5 and 7**

```bash
for p in 2 3 4 5 7; do powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase $p -RepoRoot "$PWD" -BuildRoot "$PWD/build" > "$TEMP/gate$p.log" 2>&1; echo "phase $p exit $?"; tail -3 "$TEMP/gate$p.log"; done
```
Expected: phases 2, 3, 4, 7 exit 0 with `status=pass`. Phase 5 exits 1 with its only failure the existing `candidate CANDIDATE-MISSING family=vtables` marker line. Any other failure is a regression from this task and is fixed before committing.

- [ ] **Step 5: Append the timing row and commit**

```bash
git add Physics/src/Scene.cpp Physics/src/NpJoint.cpp Physics/src/include/NpJoint.h docs/reconstruction/novodex-physics/units/revolute-contract.md docs/reconstruction/novodex-physics/evidence/unit-pilot-revolute.md
git commit -m "Construct revolute joints through the reconstructed translation units

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 11: Inventory, ledger, full verification and measurement

**Files:**
- Modify: `docs/reconstruction/novodex-physics/inventory.json`
- Modify: `docs/reconstruction/novodex-physics/gates/phase6-closure.json` (and `phase7-closure.json` / `program.json` only if the validator requires it)
- Modify: `docs/reconstruction/novodex-physics/evidence/unit-pilot-revolute.md`

**Interfaces:**
- Consumes: the row lists from `revolute-contract.md`; Task 10's outcome.

- [ ] **Step 1: Update pilot rows in the inventory**

With a short Python script (commit nothing but the resulting JSON), for every row written in Tasks 6–9 whose file is wired (or all of them, if Task 10 kept the wiring):
- `state` → `reconstructed` (rows already `reconstructed` keep their state).
- `implementation` → the `Physics/src/core/...cpp` path (the file contains the stable ID per Global Constraints).
- `source` → the same path.
- `static_proof` → `transcribed from units/<bundle>.md (<decompile label>) and checked against the Capstone listing; layout per units/revolute-contract.md`. Keep any existing `static_proof`/`dynamic_proof` text; append rather than replace.
- Do not set `dynamic_proof` unless you have evidence the row executes under the joint transcript (for example a breakpoint hit recorded in the notes); being in the call graph is not evidence.

Rows left `defer`, or rows whose wiring was reverted in Task 10, keep their current state.

Write the file back with the same formatting the inventory uses (check `git diff` is limited to the pilot rows).

- [ ] **Step 2: Update the Phase 6 ledger**

For each row moved to `reconstructed`, change its entry in `gates/phase6-closure.json` from `not_reconstructed_in_phase` to `reconstructed_not_falsified` with the same `note` text the file uses for that reason, and update `counts`. Run the validator and apply exactly the further corrections it names (program.json counts, other ledgers for pulled-in rows owned by other phases).

```bash
python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json
```
Expected: exit 0.

- [ ] **Step 3: Full verification**

From a clean build directory:
```bash
cmake -S . -B build -A Win32 --fresh && cmake --build build --config Release --target NxPhysics --clean-first
python docs/reconstruction/novodex-physics/tools/verify_public_headers.py --manifest docs/reconstruction/novodex-physics/public_header_hashes.json --root Physics/include
PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p 'test_*.py'
for p in 2 3 4 5 6 7; do powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase $p -RepoRoot "$PWD" -BuildRoot "$PWD/build" > "$TEMP/gate$p.log" 2>&1; echo "phase $p exit $?"; done
```
Expected: build succeeds; headers verify; tool tests `OK`; phases 2, 3, 4, 6, 7 exit 0; phase 5 exits 1 only on the existing vtable marker. Paste the exact summary lines into the measurement note.

- [ ] **Step 4: Regenerate the work-unit map and bundles**

Re-run the Task 1 Step 6 and Task 3 Step 5 commands so the committed map and bundles reflect the new row states.

- [ ] **Step 5: Write the measurement**

Complete `evidence/unit-pilot-revolute.md` with sections:
- `## Result` — rows and bytes moved to `reconstructed` per file; rows deferred with reasons; whether the wiring was kept.
- `## Defects found by the transcript` — from Task 10.
- `## Comparison` — the pilot's rows/bytes per hour of writing (Tasks 6–9 plus Task 10 debugging) versus a recent method-level packet. Take the comparison from `docs/superpowers/plans/2026-09-24-nxphysics-completion.md` status notes and `git log --format='%ci %s'` timestamps for the 2026-09-24 actor packets (for example the run from `4de9bd1` to `5ebb0be`), counting rows moved from the inventory diff over the same commits. State the method, both numbers, and the caveat that the pilot's bar (source + no regression) is lower than those packets' bar (behavioural differential).
- `## Verification` — the Step 3 summary lines.

- [ ] **Step 6: Commit**

```bash
git add docs/reconstruction/novodex-physics
git commit -m "Record revolute translation-unit pilot results

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```
