# The testing pass: what closed, what is held, and why

The remaining-rows pass left 184 first-party rows as listing transcriptions with a static proof only. The testing
pass gave a row the project's standard when it is **wired** (its C++ stand-in replaced in the product), **driven** by
an oracle differential in a gated target, and **falsified** by a recorded mutation inside the row's own range. Such a
row is `dynamically_gated` and its closure entry is `closed`. A row that could not be wired or driven safely, or whose
mutations were never caught, is held with the reason below; no row was forced.

**Result: 167 of the 184 rows closed; 17 held.** The records are in `raw/testing-pass-*-20261031/` (each has the
manifest `close_static_rows.py --falsified` read and the `mutations.txt` the sweeps wrote).

## Closed, by unit

| Unit | Rows closed | Gate | How driven |
|---|---|---|---|
| Internal shapes (`ShapeRows.cpp`) | 15 of 18 (001307–001341) | `NxPhysicsActorShapeMutationTests`, `NxPhysicsActorDynamicSetterTests` (staged) | already wired through the handle helpers; a dirty-queue growth family and a handle-method family were added to the shape mutation test |
| Shape handles (`NpShapeHandleRows.cpp`) | 28 of 28 | `NxPhysicsActorShapeMutationTests` (staged) | the plane/sphere/capsule/box/mesh slot tables already pointed at the rows; the new `runHandleMethodCases` family drives every public handle method |
| Mesh/mesh dispatcher | 001876 | `NxPhysicsCollisionTests`, family `contact_mesh_mesh` | oracle differential |
| `NpTriangleMeshRows.cpp` | 24 of 24 | `NxPhysicsTriangleMeshApiTests` (staged) | the DLL's `NxTriangleMesh` wrapper calls the twelve methods (`NX_NP_TRIANGLE_MESH_ROWS`); the test calls them with a scene alive |
| `TriangleMeshRows.cpp` | 16 of 16 | `NxPhysicsTriangleMeshApiTests`, `NxPhysicsConvexMeshTests` (staged) | 002239 (the mesh release), 002247, 002256, 002255 (with 002243/002245/002184/002178), 002200, 002205, 001524 and 001528 replace C++ in `TriangleMesh.cpp` (`NX_TRIANGLE_MESH_ROWS`) |
| `PMapRows.cpp` | 1 of 13 (002031) | `NxPhysicsTriangleMeshApiTests`, `NxPhysicsPMapResolution80Tests` | the penetration map's scalar deleting destructor: replaces the `loadPMap` deletion site; a mesh release reaches it through 002239's call on the map's table, whose slot 0 (the class's deleting destructor) frees through the SDK allocator |

Rows marked in `mutations.txt` with `--count-crash` were caught by a mutation that **faults the candidate while its
output differs from the oracle's** (`stdout_delta > 0`, candidate exit non-zero), not by an output difference alone.
That is a weaker detection than a clean delta, and applies to 20 of the `NpTriangleMeshRows` rows and to every
`TriangleMeshRows` and `PMapRows` row. The rows only walk structures, so a data-offset mutant faults almost every
time. The ruling is in the ledger; the sweeper's default still refuses a crash.

## Held, with reasons

**Phase 3 (5 rows).**
* 001303 (the CCD refresh) and 001345 (the scalar deleting destructor): no public API reaches them; wiring needs the
  candidate's CCD path and a deleting-destructor entry the handle tables lack.
* 001305: needs the candidate `gParameter` bound in `NxPhysicsShapeVtableTests` before `BoxShape::nxDebugRender`
  can be wired.
* 001935 and 001947: the visualisation walk passes a callback shape the candidate's walk does not; 001947 needs the
  scene globals linked into the ObjectModel-only targets.

**Phase 4 (17 rows).**
* 001988, 001992–002006, 002010–002014 (the cell-run coder): the encoder rows are interior fragments of
  phys_fn_001990 with no callable head, the decoder takes its stream in `esi`, and the whole coder keeps its cursor in
  `.data` statics where `PMap.cpp` passes a cursor struct. Wiring means rewriting `PMap.cpp`'s coder around the
  statics.
* The pruner and box-pruning rows other than the five leaves closed in the third slice (004790–005646, `PrunerSupportRows.cpp`) are unwired: none is referenced from the
  candidate, which implements the same behaviour in `opcode/Ice*.cpp` and `Novodex*.cpp` under other names. Each
  needs its candidate counterpart identified and replaced, one subsystem at a time (stream, sorters, process pool,
  factory and owner adapters, pruner slots, pool save/load), each against the gate the plan names.

**Phase 6 (11 rows)** (`Phase6Rows.cpp`, plus 004799): not started. The rows store two internal tables the candidate
models only in a harness.

**Phase 7 (29 rows)** (`Phase7Rows.cpp`, plus 004820 and others): not started. 002268–002316 is the continuous
collision sweep geometry (5,599 instructions in 37 functions); 002350 is a 1,982-byte row of the simulation step.
Wiring them replaces the candidate's step, which is the largest single change left.

## Behaviour the wiring changed

Each is pinned by the differential it ran under (oracle and candidate agree on the printed line):

* **The public triangle mesh now takes every scene's lock** and reports a refused write lock through the Foundation
  error stream, as the oracle's `NpTriangleMesh` does. Pinned by `triangle_mesh scene_calls` and
  `scene_calls_after` in `NxPhysicsTriangleMeshApiTests`.
* **`NxTriangleMesh::getSubmeshCount` returns the triangle count** (the oracle reads the internal mesh's
  `+0xc`); it returned the hull-polygon count when there was one. Pinned by `triangle_mesh api submeshes=`.
* **The internal mesh carries a real slot table** (slots 2–13 of `0x00108608`) instead of an opaque token; slots 5
  and 7 are the oracle's own rows (002200, 002205).
* **Part maps, the descriptor rebuild, the convex support maps, the hull subclass's construction and destruction and
  the penetration map's destruction** run the transcribed rows in the DLL. All are behind `NX_TRIANGLE_MESH_ROWS` /
  `NX_NP_TRIANGLE_MESH_ROWS`, defined only for the `NxPhysics` target, so the harness targets that link
  `TriangleMesh.cpp` are unchanged.

## Findings for the next pass

* **The candidate's `simulate()` does not hold the scenes' write locks.** The oracle refuses every shape setter
  called between `simulate()` and `fetchResults()` ("WriteLock is still aquired") on all five shape families and
  reports one error each; the candidate accepts them. The shape families therefore skip the refusal case. Out of
  scope here; it will show the moment the step is wired.
* **Dropped guards** in `nxSceneMarkShapeDirty` and `NxContactMeshMesh` (null aux block, unregistered id, null sink)
  cannot be reached through the public API and are unsafe in the oracle too.
* **Sweeper lessons** (all fixed in `tools/mutate_rows.py`, each with a test): MSBuild's tracker called the
  `NxPhysics.dll` link up to date after `NpShapeHandleRows.obj` changed, so earlier sweeps of that file ran
  against a stale DLL (the sweeper now deletes the DLL before each staged build); a timeout killed only the direct
  child, leaving a hung mutant holding the staged DLLs (it now kills the process tree); Windows briefly denies
  `os.replace` (retried); a sweep must rebuild the restored source (`restore_build`) and write atomically.

## Review follow-up (whole-branch review, then fixes)

Fixed, each with a test that failed first: a mesh released with a penetration map freed the map through the CRT, not
the SDK allocator (`PenetrationMap::operator delete`; pinned by `pmap_release` in `NxPhysicsTriangleMeshApiTests`);
`loadFromDesc` now matches the oracle on index-less descriptors (flip normals kept, materials passed, triangle count
`numVertices/3`) and no longer fails a mesh whose topology build reports nothing (`implicit_flip`,
`implicit_flip_counted`, `zero_area` probes); `close_static_rows.py --falsified` refuses a mutation outside the row's
address range; `mutate_rows.py` runs the gate on the unmutated source first and refuses to sweep if it already
reports a delta, treats a non-zero build exit as a failed build, and checks the restored bytes on the caught path.

Known and accepted:
* 002255 runs only when a hull is first built; the oracle runs it at the end of every successful load (convex or
  not, and on reload). That is why 002243, 002245, 002184 and 002178 are caught only by `NxPhysicsConvexMeshTests`.
* A second `loadFromDesc` on a non-convex mesh leaks the old part maps (002247 overwrites `+0x94`/`+0x98`); the
  oracle does the same, but the C++ path it replaced freed them.
* The internal table's slots 0 and 1 are null and slot 13 returns 0, so `getPMapDensity` is 0 even with a map;
  `SceneDump` now reads the mesh through real slots 3, 10 and 13 instead of the opaque token.
* The two macros are meant to be set together; `NX_NP_TRIANGLE_MESH_ROWS` without `NX_TRIANGLE_MESH_ROWS` does
  not compile. Some `#if NX_TRIANGLE_MESH_ROWS` blocks sit inside the other arm and are dead.
* `runHandleMethodCases` does not release its triangle mesh.

## Update (second slice)

The five shape-handle getters (001099, 001101, 001151, 001153, 001231) were re-swept with `--count-crash` and are
closed (`raw/testing-pass-handle-getters-20261101/`): their only mutable lines are calls and offsets, so a mutant
faults the candidate, and the output then differs from the oracle's.

## Update (third slice: the pruner leaf rows)

Five pruner support rows that are leaves (005404, 005380, 004870, 004872, 005279) are closed
(`raw/testing-pass-pruner-leaf-20261101/`) by a new oracle-differential family, `pruner_support_leaf`, in
`tests/PhysicsCollisionTests.cpp`. Each row runs at its oracle address and as the candidate's transcription over the
same fixtures; the return value, output memory and the calls a probe saw are compared. `PrunerSupportRows.cpp` is now
linked into `NxPhysicsCollisionTests`.

* The family runs under `--type3-pruner-only` (the phase 4 target `NxPhysicsType3PrunerTests`, which now registers its
  lines and raised the phase 4 coverage floor to 616) and under its own `--pruner-support-leaf-only`. It is
  deliberately **not** in the full run: the phase 3 and 7 oracle proofs pin that run's output line for line.
* The oracle-only baseline for `NxPhysicsCollisionTests` pins the source hash of `PhysicsCollisionTests.cpp`; it was
  re-pinned and every retained proof in `evidence/oracle-only-proofs/` regenerated from gates 3-7 (they are bound to
  a hash of the whole baseline).
* 005380 needed its fixture to observe a fault (`__try`) in the sentinel case, since a mutant that dereferences the
  sentinel otherwise ends the run. `mutate_rows.py --exe-arg` passes `--pruner-support-leaf-only` to a sweep.

The other 41 pruner support rows need real pruner, pool or stream fixtures (see above); 001935 and 001947 are
unchanged.

## Update (fourth slice: the pool save and load rows)

Ten pruner support rows are closed (`raw/testing-pass-pruner-stream-20261101/`): the pool save rows 005586, 005606,
005624 and 005642 and the pool load rows 005588/005590, 005608/005610 and 005644/005646. A second family,
`pruner_support_stream` (in `tests/PhysicsCollisionTests.cpp`, run by `--type3-pruner-only` and
`--pruner-support-stream-only`), runs each row at its oracle address on an oracle `MemoryStream` and as the candidate
transcription on a candidate stream, over the same objects. It compares the stream's blocks, the allocator's request
sizes, the return value and the loaded object; the loads read the oracle's own flattened save output. 100 checks, zero
mismatches. The family's lines are registered under `NxPhysicsType3PrunerTests` and the phase 4 coverage floor is 623.

Two product changes came out of it:
* `MemoryStream::flushBits`, `storeByte` and `storeDword` now return the stream, as the oracle's do (each ends
  `mov eax, esi`). The rows chain `storeDword` calls and read the stream from `eax` after `flushBits`; the candidate's
  `void` versions left garbage there, so the first run of the family faulted. The mangled names in
  `PrunerSupportRows.cpp`, `PMapRows.cpp` and `PhysicsAssetTests.cpp` were updated.
* `PhysicsAssetTests.cpp` and `PhysicsCollisionTests.cpp` changed, so both baselines were re-pinned and every retained
  oracle proof regenerated.

**004799 stays held.** The same family catches a mutation in it, but it is a phase 6 row and the only registered gate
that could discharge it is `NxPhysicsCollisionTests` in phase 7, which program.json records as pending; the ledger
will not close a row on a gate that has not passed. Closing it needs the family hosted by a phase 6 oracle target.

## Update (fifth slice: the pruner pool, the factory and the stream-to-file rows)

Eight more pruner support rows are closed (`raw/testing-pass-pruner-pool-file-20261101/`) by two oracle-differential
families in `tests/PhysicsCollisionTests.cpp`, both run by `--type3-pruner-only` (the phase 4 target
`NxPhysicsType3PrunerTests`, floor now 635) and by their own flags:

* `pruner_support_pool`: the pruner factory 004852 (with 005448 under it for the fourth type) and the process pool
  registry 004830 (register an owner, creating the pool), 004832 (remove) and 004834 (release the pool).
* `pruner_support_file`: 004790 (a stream over a file), 004793 (write the blocks to a `FILE*`) and 004801 (open a file
  in a mode and write).

Two product defects came out of it:
* **Row 004834 called the wrong release.** It called `nxOpcodeReleasePool`, which on the candidate destroys and frees
  the whole singleton; the oracle's callee releases the pool's own storage and the row then frees the object. The
  candidate side double-freed. The row now calls the pool's destructor (`??1NxPrunerProcessPool`).
* **`MemoryStream`'s block allocator took a `FILE*` for a memset.** Its third argument is a `FILE*` the data is read
  from with `fread` (`push fill, 1, size, data`, `add esp, 0x10`); it was reconstructed as a memset of the byte 1.
  Nothing had driven it, since every other caller passes zero.

Fixture notes: the families' allocator hands out zeroed memory (the constructors leave padding untouched), and the
object folds treat any word in `[0x00100000, 0x7fff0000)` as an address, because the test executable can load as low
as `0x00100000`. A null file name was left out of the 004790 cases: the oracle's CRT returns a null `FILE*` for it, the
candidate's CRT faults. 004828 (the sorters' destructors) stays held until a RadixSort can be built on the oracle side.

## Update (sixth slice: the pruner slots)

Fourteen more pruner support rows are closed (`raw/testing-pass-pruner-slots-20261101/`): the type 3 pruner's slots
005452, 005454, 005456, 005458, 005460, 005462; the static pruner's AABB query 005231; the bounded pruner's destructor
005263 and AABB query 005262; the dynamic pruner's destructor 005473, plane query 005466, AABB query 005471 and sphere
query 005470; and the tree-walk wrapper 005568.

Two families in `tests/PhysicsType3PrunerFixture.cpp` (beside the existing type 3 refit fixture), run by
`--type3-pruner-only` (the phase 4 target, floor now 637) and by their own flags:
* `pruner_support_type3`: the oracle's slots 5-8 and 0 of a type 3 pruner built by its factory, against the
  transcribed rows run on the candidate's pruner over the same four objects. Results, hit lists (as object indices)
  and the destructor's frees are compared. 17 checks.
* `pruner_support_types`: the same for types 0, 1 and 2, each queried twice (a second pass after an object moves), and
  type 1 again with 40 objects. 86 checks. The destructor check compares each side's net allocations after the
  destructor, not raw free counts: the candidate's octree build allocates more containers than the oracle's, so the
  counts differ though both sides end balanced.

The slot-to-row mapping was read from the oracle image's tables (type 0 at `0x1011b9c8`, 1 at `0x1011b9f0`, 2 at
`0x1011bbc0`, 3 at `0x1011bb98`). The candidate's row calls are wrapped in a structured-exception handler so a mutated
row that faults reads as a mismatch instead of ending the run (without it 005460 and 005462 were never caught).

Still held in this group: **005566** (the recursive octree walk: even with 40 objects the walk never recurses past the
root, so it needs a fixture that builds a deeper tree), **005479** (no caller found in the oracle), **005539** and
**005386** (reachable only through tables) and **004828** (needs a RadixSort built on the oracle side).

## Update (seventh slice: the penetration map's cell-run coder and the last pruner rows)

Fifteen more rows are closed (`raw/testing-pass-pmap-coder-and-pruner-rest-20261101/`), all registered under the phase 4
target (`NxPhysicsType3PrunerTests`, floor now 644). **Every phase 4 PMapRows row is now closed.**

* **`pmap_coder`** (`tests/PhysicsCollisionTests.cpp`; rows 001988, 001992-002006, 002010-002014). The encoder rows are
  the tail of a function whose head (001990) is C++, so they cannot be called alone. The family runs the oracle's own
  head and patches a jump over the first instruction of its tail (`0x0004cee0`), which hands control to the
  transcription; the hybrid run is compared with the oracle's unpatched run. The oracle's head reads the coder's cursor
  from `.data` statics the transcription does not write, so they are copied across after each hybrid call.
  `PMapRows.cpp`'s two statics are now exported and the file is linked into the collision harness. The decoder runs
  standalone through a small stub (`esi` = stream, cell list and resolution on the stack, plain return), on the
  oracle's own flushed encoder output. 27 checks.
* **`pruner_support_sorters`** (004828): the three RadixSort statics set in every combination; the statics afterwards and
  each side's net allocations compared. 8 checks.
* **`pruner_support_reserve`** (005479 the pruning pool's Reserve, 005539 the pruner base's scalar deleting
  destructor): 7 checks.

Still held in the pruner group: **005566** (the octree walk: its node count is zero for every population tried, up to
200 objects, so it never recurses) and **005386** (the Opcode model's stream load, which calls the vendored tree
builder). **004799** and **004820/004822/004824** stay blocked on phase 6 / phase 7 gates that have not passed.

The 001988 mutation recorded is `or eax, 0xffffffff -> or eax, 0x100000000`, a literal the assembler narrows; it changes
the cursor the reset writes and is detected.

## Update (eighth slice: the small Phase 6 rows)

Five more rows are closed (`raw/testing-pass-phase6-rows-20261101/`). A phase 6 or phase 7 row cannot close on a phase 4
gate, and phase 7 has not passed, so this slice adds a phase 6 alias gate, **`NxPhysicsPhase6RowsTests`** (a CMake custom
target over `NxPhysicsCollisionTests`, run with `--phase6-rows-only`; registered in phase 6's oracle list, floor now
1833). It runs two families:

* **`phase6_rows`** (rows 004537 the NxJoint scalar deleting destructor with the flag clear, 004759 the event list's
  constructor over the node constructor 004407, 004405 its factory including the allocation failure, 004761 its slot 5
  walking the nodes): 24 checks, each against the oracle at its address.
* **`pruner_support_stream`**, the existing family, registered under the new target as well: **004799** (the stream's
  block store) is a phase 6 row, and now closes on a phase 6 gate.

`close_static_rows.py` no longer requires a `gates/phase<N>.json` artifact for the phase (phase 6 keeps its closure file
alone).

**Retry of the two held pruner rows: both stay held.** 005566 (the octree walk): installing the adapter-notify hook
(`base+0x128500`) so inserts populate the tree corrupted the heap in the 200-object run (the Prunable copy-assign breaks
the back-pointers); the experiment was discarded. 005386 (the Opcode model's stream load) calls the vendored tree
builder and needs a loadable model.

Phase 6 is now fully closed except where noted in the ninth slice.

## Update (ninth slice: the six large Phase 6 rows)

Six more rows are closed (`raw/testing-pass-phase6-large-rows-20261101/`), under the same `NxPhysicsPhase6RowsTests`
target (phase 6 floor now 1841). The family **`phase6_large`** builds hand-made object graphs, twice from one seed, and
runs each row at its oracle address and as the transcription; every block gets an index so addresses never enter a
digest, and the callbacks and renderer are fakes that log what they are handed (116 checks):

* **004161** the record quicksort (key at +0xc), 12 arrays including heavy duplicates.
* **004159 / 004169 / 004171** the actor chain's island walk over random joint graphs (1-6 actors, up to 9 edges, null
  partners, self edges, flagged actors), through the recursive walk and the event-list factory; the digest covers the
  returned container and nodes, every actor and edge afterwards, and each side's allocation sizes and frees.
* **004401** the constraint step over two bodies and a callback object (48 cases over the flag paths, null bodies,
  zero masses and the three limit thresholds).
* **004413** the joint's debug draw through a fake renderer (the oracle's two parameter floats read from its own code
  operands and set equal on both sides), then its event list (32 cases).

The mutations recorded are the sweeper's first catch for each row, which for these rows is an argument-slot or field
offset change near the row's start; 004171 (a second row of the same function, from 0x0009ae50) has its own.
Deeper-in-body mutations were not swept.

## Update (tenth slice: the five held Phase 3 rows)

All five held Phase 3 rows are closed (`raw/testing-pass-phase3-rows-20261101/`). They were held for want of wiring; this
slice drives the listing transcriptions directly, as the Phase 6 rows were, so none of the five is wired into the
product (001303 is reached only by the CCD path the product does not run, and the others have no candidate caller).
They close on `NxPhysicsPhase6RowsTests` (a later, passed phase's gate; floor now 1848); `PrunableRows.cpp` is now linked
into the collision harness. The family **`phase3_rows`** (57 checks):

* **001303** the CCD box refresh, over the body's flag row (000738) and the prunable's world box, with the owner AABB
  hook faked on both sides (16 cases).
* **001305** the box shape's debug render, with the oracle's three parameter floats found through its own code
  operands and set equal on both sides, a fake shape (slot 10) and a fake renderer (slots 8 and 14) (32 cases).
* **001345** the shape's scalar deleting destructor with the flag clear and set; the oracle shape is zeroed, the
  candidate's is a real `ShapeBase`; the digest is the allocator's frees.
* **001947** (with 001945) the removal of an owner's prunables, against the oracle: no sweep-and-prune, a populated
  one, and a compound owner with a child.
* **001935** the visualisation node callback on its two **non-drawing** paths only (no node, a node with no box). The
  drawing path needs the oracle's `PhysicsSDK` instance and a live debug renderable; it is not driven. The recorded
  mutation (`mov al, 1` -> `mov al, 2`) is on the return value both paths share.

Two fixture lessons: the fifth stack argument of the renderer's slot-14 call is an address of a local, so it is not
logged (a digest that folded it changed every run); and the 001345 drive must not free the shape itself, or a mutated
candidate double-frees and dies without printing a mismatch.

## Update (eleventh slice: the small Phase 7 rows)

Three of the 26 Phase 7 rows in `Phase7Rows.cpp` are closed (`raw/testing-pass-phase7-rows-20261101/`): **002316** (a sweep
object's constructor from a descriptor), **002280** (the swept box merged into an output box through the sweep cache's
margin) and **002288** (the sweep cache's per-shape callback record, whose first argument arrives in `eax`). They are
driven directly, not wired. Phase 7 has not passed, but its gate list can still discharge a row: `NxPhysicsPhase6RowsTests`
is now registered in phase 7's oracle list as well (phase 7 floor 3227; phase 6 floor 1853), so these close on a gate that
runs in the row's own phase. The family is `phase7_rows` (38 checks); the sweep cache slot (`nxP7SweepCache`, now
exported from `Phase7Rows.cpp`) is set on both sides, the oracle's through the operand of its own load. `Phase7Rows.cpp`
is linked into the collision harness.

Fixture lessons: 002280 takes the output box first and the motion vector second (the listing's `[esp+0x3c]` is the
*second* stack argument, because of the saved register), and a swapped call made 13 mutations invisible; 002288's record
has two uninitialised words (offsets 0 and 0x20) that must not be logged.

**Still held (23 Phase 7 rows in Phase7Rows.cpp, plus 004820, 004822, 004824).** The remaining Phase7Rows rows are the
sweep tests proper (002268/002270 through 002310: 4 to 22 hundred instructions each), the controller creation rows
(002332 and 002336), the step row 002350 and 002400, which is an unconditional loop and cannot return. They need the
sweep cache populated with real shapes and oracle-internal geometry helpers; none was attempted.

## Update (twelfth slice: the first of the Phase 7 sweep tests)

Five more Phase 7 rows are closed (`raw/testing-pass-phase7-sweep-20261101/`), again on `NxPhysicsPhase6RowsTests`
(phase 6 floor 1860, phase 7 floor 3234), in a new family **`phase7_sweep`** (752 checks):

* **002268 / 002270** the oriented-box overlap test (one function at 0x00056690): two rotation matrices, two centres
  and extent triples, and a flag for the nine edge-cross axes; the operands arrive in `eax`, `ecx`, `edx`, `edi` and on
  the stack, so it is called through a stub. 160 random pairs (98 overlapping).
* **002274** the loop of the ray/AABB slab helper, entered by the jump of its head (002272, a closed row with a C++
  body). The stub is the head's instructions with the jump taken to the transcription; the oracle side is the whole helper
  at 0x00056ad0. 400 cases.
* **002276** and **002278** two register-ABI distance tests. Their operands are drawn as blocks of floats behind every
  pointer register and stack word; the digest is the result and every block afterwards. **This is weaker than the others**:
  the operand roles were inferred from the listings and a call site, not from the callers' meaning, so the inputs may
  reach only part of each routine. Both were caught on their first mutation, and the digest was made deterministic by
  masking 002278's result to `al` (its upper bytes are garbage).

**Held, with the reason found.** 002282, 002284 and 002286 (the swept oriented-box test, one function) run to the end on
both sides, and 124 of 128 random cases match, but four differ in the last bits of the result. The cause is not in the
row: the vendored `OBB::ComputePoints` (005201, still `reconstructed`) is not bit-identical to the oracle's on generic
rotations (2812 of 3000 random boxes differ; `InvertPRMatrix` and `ComputeWorldEdgeNormal` were exact). Until that helper
is exact the swept test cannot be closed, and 002296 (which calls it) waits on the same helper. The drive was removed.

Two integration defects were found and fixed on the way, both in `Phase7Rows.cpp`:
* The row aliased its slab helper to `NxRayAABBSlab` as if it took all six arguments on the stack. The oracle's helper
  (002272) takes the **ray direction in `ecx`** (the fourth parameter) and five on the stack; the alias is now an adapter
  that rebuilds the cdecl call in a frame (the callee clobbers `edx`, so the return address cannot wait there).

**Not attempted:** 002290/002292/002294 (the triangle sweep), 002296, 002298, 002300/002302, 002304, 002306, 002308,
002310 (all call vendored helpers or each other and need real shapes), 002332/002336, 002350, 002400 (an endless loop)
and 004820/004822/004824.

## Update (thirteenth slice: the exact ComputePoints and the triangle sweep)

Six more Phase 7 rows are closed (`raw/testing-pass-phase7-sweep2-20261101/`), in the same `phase7_sweep` family
(now 1080 checks; phase 6 floor 1866, phase 7 floor 3240).

* **The cause of the swept-box mismatch is fixed.** The oracle's `OBB::ComputePoints` (005201) keeps some partial sums in
  x87 registers and stores others, so no source-level rewrite reproduces it. It is now a listing transcription in
  `OpcodeVendoredRows.cpp`, with the upstream body guarded in `novodex/Ice/IceOBB.cpp`, like the neighbouring vendored
  rows. The probe that found the difference now reports it exact. **002282, 002284 and 002286** (the swept oriented-box
  test, one function) then matched on all 128 cases and were caught by mutations in each row's range.
* **002290, 002292 and 002294** (the triangle-against-box sweep): seven stack words, called through a stub. Two lessons
  that cost a day: the second-from-last operand pair is *not* all pointers (argument 2 is an integer, and a block address
  there made the result depend on bit 3 of the address, so the digest changed from process to process), and the
  stack below the call is now zeroed before each call. As for 002276 and 002278, the other operands are float blocks of
  inferred roles, so reach is partial; 002292's recorded mutation is detected in one case only.

The vendored coverage for 005201 was `discrete`; `NxPhysicsThirdPartyTests` reports its class again, so its pinned line
was refreshed if it changed (see the commit).

**Still held (17 rows).** 002296 (calls 002282/2290 and now unblocked, not attempted), 002298, 002300/002302, 002304,
002306, 002308, 002310, 002332, 002336, 002350, 002400 (an endless loop), 004820, 004822, 004824, 005566 and 005386.

## Review of the whole pass, and what it changed (fourteenth slice)

A fresh reviewer read the 67 commits of this pass against the tree. No Critical findings; four Important, eight Minor.
Rulings and fixes:

* **I1 (static proofs no longer reproduced) -- fixed.** The compare scripts for PMapRows, PrunerSupportRows and
  Phase7Rows still carried the old symbol names (`storeByte`, `storeDword`, `flushBits` now return the stream; the pool
  destructor; the slab adapter). Their EXPECTED tables are updated; all three report `unexpected=0` again and the
  instruction bodies were never different.
* **I2 ("wired" overstated) -- fixed in the record.** `close_static_rows.py` wrote "wired and driven" for every row. 82
  rows (every `NxPhysicsType3PrunerTests` and `NxPhysicsPhase6RowsTests` closure) are driven directly in the harness and
  not called by the product. Their notes now say so; the tool takes `"wired": false` in a manifest (tested). **Read the
  headline accordingly: 167 rows are driven and falsified; about 85 of them are also wired into the product.**
* **I3 (mutations prove entry, not the body) -- measured.** `mutate_rows.py --survey --skip 2 --limit 8` tries mutations
  spread over the row, past any catch, and lists the survivors. Results (survivors / tried):
  Phase 7 sweep rows: 002292 0/8, 002284 1/8, 002286 1/8, 002268 0/8, 002270 1/8, 002294 1/8, 002290 1/8, 002282 1/8,
  002276 0/8, 002278 0/8 (6 of 80). Phase 7 small rows: 002316 1/8, 002280 1/8, 002288 1/8. Phase 6 large rows: 004161
  3/8, 004159 5/8, 004169 5/8 (6/8 before the drive was widened), 004171 4/8, 004401 0/8, 004413 2/8. Phase 6 small rows:
  004537 1/1, 004759 0/2, 004405 4/7, 004761 7/8. Phase 3: 001303 2/8, 001305 5/8, 001345 3/3, 001947 4/8.
  The reviewer's worry was right for the older families and wrong for the Phase 7 sweep rows. For the weak ones I widened
  the 004161/004169 drives (120 sorts with six key patterns; 400 actor graphs of up to 12 actors and 24 edges): the same
  mutants survive, so they are not limited by the data. Reading them: many are equivalent (a stack slot beside the right
  one that is never read; an allocator vtable slot next to `malloc`), but not all: 004169's flag branches
  (`je L9ae41`, `jne L9ae03`), 004761's children path (the `+0x18` joint owner needs a real joint), 004537's
  flag-set free (needs the oracle's own CRT heap), 001305's lower draw calls and 001947's second arm are **not reached**.
  **Rows with more than half their mutants surviving are only weakly falsified: 004159, 004169 (5/8), 004761 (7/8),
  001345 (3/3), 004537 (1/1), 001305 (5/8).** They meet the recorded standard (one mutation inside the row is caught) but
  a deeper transcription bug there would probably pass. A later pass should drive those paths with real objects.
* **I4 (the "passed gate" rule) -- ruling.** The tool requires the gate to be registered in the row's phase or a later
  one; it does not require that phase to be `pass` in program.json, and 14 phase 7 rows closed on
  `NxPhysicsPhase6RowsTests` through phase 7's list while phase 7 is `pending`. This is by design of the slices after the
  fourth (a gate that runs in the row's own phase discharges it), but the earlier slice text saying "a later, passed
  phase's gate" is imprecise for the phase 7 rows. `run_phase_gate.ps1 -Phase 6` and `-Phase 7` were run at the final
  floors (1866 and 3240) on every slice and passed; there is no oracle-only proof for the alias target, as for the
  type-3 one.
* **M3 -- fixed:** 004413 now checks the opcode bytes before reading its parameter operands.
* **M5 -- deferred:** inventory still records 005201 (`OBB::ComputePoints`) as `reconstructed` from the upstream source;
  it is now the listing transcription in `OpcodeVendoredRows.cpp`. Its state and `vendored_coverage.csv` line
  (`ice_obb=discrete`) were not updated.
* **M1, M2, M4, M6, M8 -- deferred:** summary lines that omit a few closed rows (005452, 005568, 005590, 005610, 005646, and
  the pmap coder's unnamed rows); early `return nxFail(...)` paths that leave allocators pointing at dead stack objects;
  the harness's PenetrationMap built with `new` and freed through the SDK allocator; the scrub that blanks all words in
  the address range; the sweeper's hard-coded oracle path.
* **M7:** the "Held, with reasons" section near the top of this file describes the state after the first slices; the
  latest status is the last update below it.

## Strengthening the weak rows (fifteenth slice)

**The survey over-counted survivors.** `mutate_rows.py` ignored `--count-crash` for harness targets, so a mutant that
faulted the harness before it printed its summary line was scored as a survivor. `detect_harness` now scores a fault-range
exit (not a launch failure, exit 127) with no summary line as a detection (tested). Re-run over the same rows
(survivors / tried, 8 mutants each unless noted):

* Phase 6 large: 004161 2, 004159 2, 004169 **0**, 004171 3, 004401 0, 004413 0.
* Phase 6 small: 004537 **0** (1 candidate), 004759 0 (2), 004405 1 (7), 004761 3.
* Phase 3: 001303 0, 001305 4, 001345 **0** (3 candidates), 001947 0.
* Phase 7 small: 002316 0, 002280 0, 002288 0.
* Phase 7 sweep: 002292 0, 002284 0, 002286 1, 002268 0, 002270 1, 002294 0, 002290 1, 002282 1, 002276 0, 002278 0.

So the earlier "weakly falsified" list was mostly crashes. The two rows that really were weak got stronger drives:

* **004761** gains a joint owner at `+0x18` (a zeroed 0x400-byte object whose tail is filled, so the row's two stores and
  its slot-6 call run on both sides and the whole object is compared): survivors 3 -> **0**. 12 cases (30 checks in the
  family).
* **001305** logs the polygon call's first eleven words, uses distinct random shape floats and negative parameter values
  (so the `test ah` branches are taken both ways): survivors 4 -> **2** of 14, both reads of a stack slot beside the one
  in use.

Residual survivors after this slice are 14 of about 200 mutants tried (7%): single mutations of a stack slot or allocator vtable slot next to
the right one (often equivalent), and 002270's `test ah, 5 -> 6`, 002290's `shr ebx, 3 -> 4`, 002282's `push 1 -> push 2`
(a flag the drive never varies).

**002296 attempted and not closed.** It needs its operands in the right shapes: the eax holder is a triangle array (a byte
size at +4, the data at +8) and arguments 1 and 2 are not plain float blocks (the first fault was an integer index array
read as a pointer; the second is a loop reading past a block's end from a stack word that is not one of the drive's
blocks). The row stays held with 002298 (its wrapper).
