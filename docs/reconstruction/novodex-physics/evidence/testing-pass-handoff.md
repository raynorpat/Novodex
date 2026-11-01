# The testing pass: what closed, what is held, and why

The remaining-rows pass left 184 first-party rows as listing transcriptions with a static proof only. The testing
pass gave a row the project's standard when it is **wired** (its C++ stand-in replaced in the product), **driven** by
an oracle differential in a gated target, and **falsified** by a recorded mutation inside the row's own range. Such a
row is `dynamically_gated` and its closure entry is `closed`. A row that could not be wired or driven safely, or whose
mutations were never caught, is held with the reason below; no row was forced.

**Result: 85 of the 184 rows closed; 99 held.** The records are in `raw/testing-pass-*-20261031/` (each has the
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

**Phase 4 (54 rows).**
* 001988, 001992–002006, 002010–002014 (the cell-run coder): the encoder rows are interior fragments of
  phys_fn_001990 with no callable head, the decoder takes its stream in `esi`, and the whole coder keeps its cursor in
  `.data` statics where `PMap.cpp` passes a cursor struct. Wiring means rewriting `PMap.cpp`'s coder around the
  statics.
* The pruner and box-pruning rows (004790–005646, `PrunerSupportRows.cpp`) are unwired: none is referenced from the
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
