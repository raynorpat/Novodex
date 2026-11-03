# Falsifying the remaining reconstructed rows: design

Sub-project 2 of the four-part completion program (Phase 7 closure, done; **falsifying the remaining reconstructed rows**; wiring the
verified rows into the shipping DLL; the Phase 8 audit). The program's success criterion is that the shipping `NxPhysics.dll` is the
oracle and every code row reaches the terminal state in the Phase 8 audit. This spec covers one step of that: giving every code row
that is not yet `dynamically_gated` a mutation that a gate caught, or a recorded reason it cannot have one.

## Goal

Every code row now at `reconstructed`, `discovered` or `statically_reviewed` (1,365 rows, 569,544 bytes at the time of writing)
ends in exactly one of two states: `dynamically_gated` with a recorded mutation **inside the row's own compiled range** that a gate
caught, or deferred in its phase's closure ledger with a reason `validate_inventory.py` accepts. No row is forced: a row whose
mutants mostly survive is deferred, not closed.

## What the rows are

| group | rows | notes |
|---|---|---|
| vendored qhull | 455 | third-party code compiled from `External/qhull`; gated by the vendored "outcome-exact" families |
| vendored opcode | 181 | same, from `External/opcode` |
| first-party, mapped to a C++ function and already driven (`implementation` and `dynamic_proof` set) | 481 | the shipped code itself; the gap is only an aimed mutation |
| vendored, mapped and driven | 546 (counted with the two rows above) | same |
| first-party, mapped, no dynamic proof | 112 (+40 vendored) | need a driver first |
| first-party, no implementation mapping | about 130 (+30 vendored) | find the mapping or defer |
| `discovered` (19 vendored) and `statically_reviewed` (7) | 26 | classify |
| carried over: 003649, 005386, Phase 7 deferrals | 14 | known hard cases; revisit last |

Largest first-party files: `PrunerOctree.cpp` 35 rows, `SceneDump.cpp` 27, `IceMeshBuilder2.cpp` 25, `QhullHost.cpp` 24,
`ContactConvexHeightfield.cpp` 17, `D6Joint.cpp` 16, `SphericalJoint.cpp` 15, the thirteen-row `Np*Joint.cpp` wrappers (one per
joint family), `ContactGeneration.cpp` 14.

## Methods

**A. Naked-asm transcriptions** (`Physics/src/*Rows.cpp`): the existing `mutate_rows.py`, unchanged.

**B. C++ rows: a new `mutate_cpp.py`.** For a row with an inventory `implementation_symbol` and `source`:
1. Locate the function's source span from the symbol and the linker map.
2. Apply operator-level mutations inside that span only (`<` with `<=`, `+` with `-`, `==` with `!=`, an integer constant plus one,
   two arguments swapped, a branch negated).
3. **Range check.** Build the candidate DLL before and after, and require every changed byte to fall inside the row's compiled
   range (the function's rva and size from the inventory). A mutation that changed nothing, or changed an inlined callee elsewhere,
   is discarded. This is what keeps "a mutation inside the row's own range" true for compiled C++.
4. Rebuild the gated target and run the row's family; compare against the oracle baseline exactly as `mutate_rows.py` does, with
   `--count-crash` available.
5. Write a log that `close_static_rows.py --falsified` already reads (gate, family, mutation, detected, `wired`). For C++ product
   rows `wired` is true: the row is the shipped code.

**Batching.** One rebuild per mutant would take weeks. The tool mutates one function in each of N rows whose gate families are
disjoint, builds once, runs once, and attributes each detection by family; a family that fails is bisected. N is chosen from the
measured build and gate time of the pilot.

**C. Vendored qhull and opcode:** the same tool on the upstream sources under `External/`, with the byte-range check, and the
existing vendored families as the gate. Where a row's function is fully inlined or a template instantiation with no stable
range, the row is deferred `homeless_shared_code` or `not_independently_falsifiable` with the evidence.

**D. Rows without a driver or mapping:** first look for a family that already reaches them (cdb hit counts, as in Phase 7, where
rows judged unreachable were in fact only unhooked); then write the smallest driver; otherwise defer with the reason.

## Order

1. Build and pilot `mutate_cpp.py` on the joint families (D6, spherical, revolute, pulley, distance, and the thirteen `Np*Joint`
   wrappers; about 200 rows), where fixtures and staged-pair gates exist. The pilot fixes N and the manifest format.
2. Collision, contact and mesh rows (`ContactConvex*`, `ContactMesh*`, `Distance`, `TriangleMesh*`, `IceMeshBuilder2`).
3. Pruner, `SceneDump`, `QhullHost` and the remaining first-party rows.
4. Vendored opcode, then vendored qhull (one family at a time).
5. Rows needing a driver or a mapping; the `discovered` and `statically_reviewed` rows.
6. The carried-over rows (003649, 005386, Phase 7 deferrals).

Each step ends with the full gate cycle (phases 3-7 exit 0), regenerated `work_units.json` and `completion-backlog.json`, tooling
tests green, one commit with explicit paths, and a local fast-forward merge.

## Constraints

* Never push; fast-forward merge to `main` locally; explicit-path `git add`; leave unrelated untracked files and `stash@{0}` alone.
* Commit trailer `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.
* Fixtures and logs must not contain text from the capturing process (environment, paths, names): see
  `evidence/phase7-closure-audit.md`, Review follow-up.
* A mutation counts only if the range check passes and the control run (unmutated) is exact.
* Time-box each family; a spike that does not reach a row ends in a recorded deferral.

## Risks

* Inlining and templates may leave many rows without a standalone range; the byte-range check will expose them early, in the pilot.
* Equivalent mutants (changes with no observable effect) can never be caught; the operator set is varied and the threshold for
  closing is at most two survivors per sixteen tried, as in Phase 7.
* Gate cycles take about twelve minutes; batch size must keep the total number of cycles in the tens, not hundreds.
* Some vendored rows may be better closed by the existing matcher evidence than by mutation; if so that is a ruling, recorded.
