# The testing pass: what closed, what is held, and why

The remaining-rows pass left 184 first-party rows as listing transcriptions with a static proof only. The testing
pass gave a row the project's standard when it is **wired** (its C++ stand-in replaced in the product), **driven** by
an oracle differential in a gated target, and **falsified** by a recorded mutation inside the row's own range. Such a
row is `dynamically_gated` and its closure entry is `closed`. A row that could not be wired or driven safely, or whose
mutations were never caught, is held with the reason below; no row was forced.

**Result: 148 of the 184 rows closed; 36 held.** The records are in `raw/testing-pass-*-20261031/` (each has the
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
