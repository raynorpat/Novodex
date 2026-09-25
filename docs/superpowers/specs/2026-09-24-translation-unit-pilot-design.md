# Translation-unit work units and the revolute joint pilot — design

**Date:** 2026-09-24
**Status:** Draft for review
**Relates to:** `docs/superpowers/plans/2026-09-24-nxphysics-completion.md` (M0 backlog, M5 joints)

## 1. Problem

Reconstruction currently advances one public method at a time. Each packet
rediscovers the layouts, shared helpers and inlined code that the rest of its
original source file already carries, and many packets add behavioural checks
without moving any row's state. The shipped image was linked from contiguous
object files; 57 of them are named by `NX_ASSERT` `__FILE__` strings. Working a
whole original translation unit at a time should amortise that rediscovery.

This design builds the tooling to organise work by translation unit, and runs
one pilot unit end to end to measure whether writing source that way is faster.

## 2. Scope

**In scope**

1. `work_units.json`: a map of every executable row to a work unit.
2. Per-unit reference bundles, generated from committed oracle data.
3. A pinned Ghidra supplement for pilot rows that have no committed decompile.
4. The pilot: real product source for `RevoluteJoint.cpp` and
   `NpRevoluteJoint.cpp`, plus the shared `Joint.cpp` base rows the revolute
   constructor needs, wired into scene joint creation.
5. A measurement note.

**Out of scope (decided)**

- Per-row falsification (`differential_falsified`) and new behavioural
  coverage. The pilot is *source first*: rows reach `reconstructed`, never
  higher. Verification proves no regression, not correctness of new rows.
- Changing phase ownership, `phase_provenance`, or any closure ledger rule.
- Removing or rewriting the parameterised row models in `ObjectModel.cpp`.
- Inferred boundaries inside unnamed gaps beyond what the map records; the
  vendored OPCODE/qhull gaps are a separate effort.

## 3. Measured facts this design rests on

- The named-unit spans (first to last asserting row) cover ~200 KB of ~938 KB of
  product code; ~728 KB lies in unnamed gaps. Two gaps (`IcePrunable` →
  `OPC_MeshInterface`, 210 KB; `Controller` → `fluids\Fluid`, 171 KB) are mostly
  Phase 4.
- `oracle/ghidra/manifest.json` carries `decompiler_c` per Ghidra function.
  Ghidra made functions for 2,527 of 22,081 Capstone entries.
- RevoluteJoint.cpp's evidenced span is `0xa8d40`–`0xac630`; its rows extend at
  least `0xa8d20`–`0xac700` and probably through `0xad0b0` (`phys_fn_004374`,
  which the revolute internal dispatch table `phys_data_002684` names beside
  `0xa8d20`). NpRevoluteJoint.cpp's span is `0xb2d10`–`0xb32b0`, with
  neighbouring rows `0xb2c80`–`0xb2cd0` and `0xb3340`–`0xb33a0`.
- Three of the four largest pilot rows have no committed decompile:
  `phys_fn_004356` (`0xa9650`, 2,303 B), `phys_fn_004360` (`0xaa060`, 4,460 B),
  `phys_fn_004364` (`0xab840`, 3,326 B).
- `phys_fn_004366` (`0xac540`) is called from `phys_fn_000665` (`0x142c0`, Scene,
  `dynamically_gated`) and reaches nearly every other RevoluteJoint.cpp row. It
  also calls Joint.cpp base rows `004064 004066 004093 004097 004123 004127
  004129 004135` and `004389 004391 004393`, all `discovered`.
- The 16 pilot rows already marked `reconstructed` are implemented as
  parameterised models shared across joint families (for example
  `nxLockedCopyAndFlag` covers 004342/004346/004350), called by tests beside the
  oracle. The candidate DLL has no revolute object: `NpJointVtable::isRevoluteJoint()`
  returns 0 and `createJoint` builds a generic joint.
- `CMakeLists.txt` globs `Physics/src/*.cpp` non-recursively.

## 4. Components

### 4.1 `tools/work_units.py` → `work_units.json`

Reads `inventory.json`, `oracle/ghidra/manifest.json` and
`oracle/capstone/manifest.json`. Emits one record per unit:

| Field | Meaning |
|---|---|
| `unit` | `__FILE__` name, or `gap:<prev>..<next>` |
| `evidenced_span` | first/last row carrying the unit's `__FILE__` reference (null for gaps) |
| `inferred_extent` | rows assigned to the unit, see rule below |
| `ambiguous_rows` | rows the rule could not assign, listed and left out of any unit |
| `rows`, `bytes` | per `state`, code rows only |
| `phases` | row count per owning phase |

**Extent rule.** A named unit's extent is its evidenced span, grown outward one
row at a time while the next row has a direct call edge into the unit's current
rows and none into the neighbouring unit's rows. The left unit grows first, then
the right unit over what remains. Rows neither side claims are the
`ambiguous_rows` of a `gap:<left>..<right>` unit. A row with no edge into either
unit is ambiguous, not assigned: unconnected code (for example the vendored
OPCODE region) must not flow into its neighbour. The rule is descriptive: it
writes nothing into `inventory.json`.

Every executable code row appears in exactly one unit's extent or exactly one
`ambiguous_rows` list; the tool fails otherwise. Span checks reuse
`reconcile_analysis.translation_unit_spans` so the two cannot disagree.

### 4.2 `tools/unit_bundle.py` → `units/<unit>.md`

For one unit, writes a reference document with, per row in address order:
stable ID, RVA, size, state, current `source`, Ghidra prototype and calling
convention, stack purge, callers and callees (direct edges), indirect-call sites
from Capstone, referenced strings and assert file/line, and the decompile — from
`decompiler_c`, else from the supplement (4.3), else Capstone disassembly marked
as such. The header lists dispatch tables whose slots target the unit's rows and
the unit's external dependencies grouped by owning unit.

Bundles are regenerable and committed so review can see what the writer saw.

### 4.3 Ghidra supplement for rows without a decompile

`ghidra/DecompileSupplement.java`, run through the pinned headless Ghidra with
the existing type application (`ApplyPhysicsTypes.java`,
`physics_type_inputs.json`), creates a function at each requested Capstone
entry lacking one and records its decompile. Output:
`oracle/ghidra/supplement.json` with the Ghidra version, analysis-options hash,
requested RVAs and per-row `decompiler_c`/status. The main manifest is not
modified. The pilot requests `0xa9650`, `0xaa060`, `0xab840`, the small pilot
rows without functions (`0xa8d20`, `0xa8f10`, `0xa8fb0`, `0xa8fc0`,
`0xb2cc0`, `0xb3270`), and any row the extent adds that lacks one.

### 4.4 Pilot source

- `Physics/src/core/RevoluteJoint.cpp`, `Physics/src/core/NpRevoluteJoint.cpp`,
  `Physics/src/core/Joint.cpp` (only the base rows the revolute constructor and
  its callees need), and private headers under `Physics/src/include/core/`.
  Functions appear in oracle address order within each file, each preceded by a
  one-line stable-ID/RVA comment in the existing style.
- `CMakeLists.txt`: add `Physics/src/core/*.cpp` to `PHYSICS_SOURCES`.
- `NpScene::createJoint` / internal `Scene` joint creation: construct the new
  revolute object for `NX_JOINT_REVOLUTE`; other joint types keep the generic
  path. The public `NxRevoluteJoint` interface is implemented by
  `NpRevoluteJoint` so `isRevoluteJoint()` returns the object for revolute
  joints only.
- Existing parameterised models in `ObjectModel.cpp` stay. Product rows that the
  models already describe are written natively in the new files; the model's
  doc comment gains a pointer to the product row.
- Public headers are not edited.

### 4.5 Inventory update

Pilot rows with product source move to `reconstructed` with `source` set to
the new file path. Rows already `reconstructed` update only `source`. No row is
promoted past `reconstructed`. The Phase 6 closure ledger regenerates from the
inventory; its counts are the only ledger change.

## 5. Verification

All must pass before the pilot is reported done:

1. Clean Win32 Release configure and build of `NxPhysics`.
2. `verify_public_headers.py`: all pinned headers unchanged.
3. `NxPhysicsJointTests` staged-pair transcript byte-identical to the committed
   oracle transcript. Because revolute creation now runs the new code, this is a
   regression check on the constructor, descriptor load and accessors it
   exercises.
4. Phase gates 2, 3, 4, 6 and 7 pass; Phase 5 fails only on its existing
   final-vtable marker.
5. `validate_inventory.py` and the tool test suite pass, including new tests for
   `work_units.py` (partition completeness, span agreement, ambiguous-row
   reporting) and `unit_bundle.py` (fallback order).

A transcript difference is treated as a defect in the new source and is fixed,
not normalised. If a difference cannot be resolved within the pilot, revolute
creation reverts to the generic path, the new source stays unwired, the affected
rows stay `discovered`, and the difference is recorded as a finding.

## 6. Measurement

`evidence/unit-pilot-revolute.md` records: rows and bytes written per file;
wall-clock per step (tooling, supplement, bundle, writing, build/fix,
verification); defects found by the transcript; and the same figures for a
comparable recent method-level packet from the Phase 5 notes, so the comparison
uses recorded numbers rather than impressions.

## 7. Risks

- **Base-class depth.** The Joint.cpp rows pulled in may call further unstarted
  rows. The pilot includes only what the revolute constructor path needs;
  deeper callees stay behind the existing generic behaviour where possible, and
  any that cannot are listed in the measurement note.
- **Indirect calls.** Solver-facing rows dispatch through the internal table
  `phys_data_002684`. Targets are resolved per receiver type from that table,
  not assumed from direct edges.
- **x87 behaviour.** The large rows are floating-point heavy. The transcript
  exercises only what joint creation reaches; stepping-time behaviour is not
  verified by this pilot.
- **Extent inference.** A wrong extent assigns rows to the wrong file. Pilot
  extents are checked by hand against the dispatch table and call edges before
  source is written.

## 8. Success criteria

- `work_units.json` partitions every code row, and bundles exist for the two
  pilot units.
- Every code row in the pilot units' inferred extents, and the Joint.cpp base
  rows pulled in, has product source and is `reconstructed`.
- All verification in section 5 passes with revolute creation wired to the new
  code.
- The measurement note exists with the comparison figures.
