# Remaining-rows pass: hand-off to testing

The remaining-rows pass (`docs/superpowers/plans/2026-10-30-remaining-rows.md`, spec
`docs/superpowers/specs/2026-10-30-remaining-rows-design.md`) took the 270 rows that were `discovered` on 2026-10-30.
Every first-party row (184) is now `reconstructed` with a static proof: a naked listing transcription whose
listing-compare script finds every instruction equal and every call mapped. None of them has a differential or a
mutation aimed at it, and the testing pass starts here. The 86 vendored rows were taken by the vendored-rows pass (below).

## The units

| Evidence | Rows | Source |
|---|---:|---|
| `evidence/phase3-mesh-mesh-contact.md` | 1 | `Physics/src/ContactMeshMeshRow.cpp` |
| `evidence/phase3-prunable-rows.md` | 2 | `Physics/src/opcode/PrunableRows.cpp` |
| `evidence/phase3-shape-handle-rows.md` | 28 | `Physics/src/NpShapeHandleRows.cpp` |
| `evidence/phase3-shape-rows.md` | 18 | `Physics/src/ShapeRows.cpp` |
| `evidence/phase4-np-triangle-mesh-rows.md` | 24 | `Physics/src/NpTriangleMeshRows.cpp` |
| `evidence/phase4-pmap-rows.md` | 13 | `Physics/src/PMapRows.cpp` |
| `evidence/phase4-pruner-support-rows.md` | 46 | `Physics/src/opcode/PrunerSupportRows.cpp` |
| `evidence/phase4-triangle-mesh-rows.md` | 16 | `Physics/src/TriangleMeshRows.cpp` |
| `evidence/phase6-rows.md` | 10 | `Physics/src/core/Phase6Rows.cpp` |
| `evidence/phase7-rows.md` | 26 | `Physics/src/Phase7Rows.cpp` |

## Rows by the test target that drives their nearest gated caller

The nearest `dynamically_gated` caller within three call levels, and the test target its dynamic proof names. A
family there is the natural place to drive the row.

* **NxPhysicsActorDynamicSetterTests** (3): 001309, 001313, 001321.
* **NxPhysicsActorShapeMutationTests** (9): 001307, 001331, 001333, 001335, 001337, 001339, 001341, 004852, 005448.
* **NxPhysicsAssetTests** (4): 001988, 004790, 004793, 004801.
* **NxPhysicsCollisionTests** (1): 005479.
* **NxPhysicsObjectLayoutTests** (2): 001305, 001325.
* **NxPhysicsPopulatedSceneTeardownTests** (2): 004828, 004834.
* **NxPhysicsSceneRaycastTests** (3): 004830, 004832, 005404.
* **NxPhysicsSceneVisualizeTests** (1): 005279.
* **NxPhysicsSimulationTests** (5): 001303, 002350, 004169, 004820, 004824.
* **NxPhysicsTriangleMeshApiTests** (1): 002239.
* **(no driven caller within three call levels)** (153): 001099, 001101, 001103, 001105, 001131, 001133, 001135, 001137, 001139, 001141, 001143, 001145, 001147, 001151, 001153, 001157, 001167, 001181, 001211, 001213, 001215, 001217, 001225, 001227, 001229, 001231, 001233, 001235, 001311, 001317, 001319, 001327, 001345, 001524, 001528, 001876, 001935, 001947, 001992, 001994, 001996, 001998, 002000, 002002, 002004, 002006, 002010, 002012, 002014, 002031, 002077, 002095, 002097, 002098, 002100, 002101, 002103, 002104, 002106, 002108, 002110, 002112, 002114, 002116, 002118, 002120, 002122, 002124, 002126, 002128, 002130, 002132, 002134, 002136, 002138, 002178, 002184, 002190, 002192, 002194, 002200, 002205, 002243, 002245, 002247, 002255, 002256, 002268, 002270, 002274, 002276, 002278, 002280, 002282, 002284, 002286, 002288, 002290, 002292, 002294, 002296, 002298, 002300, 002302, 002304, 002306, 002308, 002310, 002316, 002332, 002336, 002400, 004159, 004161, 004171, 004401, 004405, 004413, 004537, 004759, 004761, 004799, 004822, 004870, 004872, 005231, 005262, 005263, 005380, 005386, 005452, 005454, 005456, 005458, 005460, 005462, 005466, 005470, 005471, 005473, 005539, 005566, 005568, 005586, 005588, 005590, 005606, 005608, 005610, 005624, 005642, 005644, 005646.

## Rows with indirect call sites

These need a live table or object to drive (87): 001103, 001105, 001131, 001133, 001135, 001137, 001139, 001141, 001143, 001145, 001147, 001157, 001167, 001181, 001211, 001213, 001215, 001217, 001225, 001227, 001229, 001233, 001235, 001305, 001307, 001325, 001327, 001333, 001335, 001337, 001339, 001341, 001345, 001528, 001876, 001935, 001947, 002031, 002077, 002097, 002100, 002103, 002106, 002110, 002114, 002118, 002126, 002130, 002134, 002138, 002178, 002190, 002192, 002194, 002205, 002239, 002243, 002245, 002247, 002255, 002288, 002302, 002304, 002308, 002310, 002336, 002350, 004171, 004401, 004405, 004413, 004761, 004820, 004828, 004830, 004834, 004852, 005263, 005279, 005386, 005454, 005473, 005479, 005539, 005588, 005608, 005644.

## What is transcribed but not wired

Most units are not wired: the candidate keeps its own C++ for those paths, so the naked rows are in the DLL but
unreferenced. The wired ones are the internal shape rows (through the `NpActor.cpp` handle helpers and
`nxSceneMarkShapeDirty`), the public shape-handle rows (through the handle tables) and 001876 (through
`NxContactMeshMesh`). Re-pointing was deferred on purpose for:

* 001305: `BoxShape::nxDebugRender` stays C++ until `NxPhysicsShapeVtableTests` binds the candidate's parameters rather than the oracle's `.data`;
* 001309: `ShapeBase::nxShapeGlobalPose` stays C++ because the ObjectModel-only test targets do not link the scene globals;
* 001945/001947: `NxScenePrunerOwnerDestroy` stays C++ for the same reason;
* 001935: the candidate's visualisation walk passes its own callback.

## Guards the wired forwards no longer have

Two forwards replaced C++ bodies whose defensive checks the oracle's rows do not make. The forwards behave as the
oracle does, so the testing pass should drive these inputs deliberately:

* `nxSceneMarkShapeDirty` (`Scene.cpp`, now 001325). The C++ returned early on:
  * a null scene;
  * a null aux block at `[scene+0x48]`;
  * a shape id at or past the flag array's end.

  001325 only tests the owner at `shape+4`. A shape that outlives its scene's aux block, or one whose id was never
  registered, now writes where the old code returned.
* `NxContactMeshMesh` (`ContactMeshMesh.cpp`, now 001876). The C++ returned early on a null shape, a null sink or a
  null geometry image. The forward passes them through to 001876, which does not check them.

## Behaviour the re-pointed setters now share with the oracle

The box and capsule handles' `setFlag` and the local and global pose setters run rows that test and set the API
reentry flag (`gNxApiReentry`). A call made while something else holds that flag, such as `ImplicitMesh.cpp`'s
locked sections (lines 41 and 220), is now refused with the reentry report, where the C++ setters ran it.

## Hard rows

None. Every first-party row was transcribed.

## Vendored rows

The vendored-rows pass (`evidence/vendored-correspondence.md`, "Vendored-rows pass (2026-10-30)") transcribed all 86
and recorded 67 with an exact execution. These 19 stay `discovered`, each transcribed and listing-equal, and are
for the testing pass:

* `qh_detvridge3` (002699, 002701): 3-d Delaunay/Voronoi only; those doubles diverge.
* `qh_degen_redundant_facet` (002900, 002902): no driven input reaches it.
* `qh_maydropneighbor` (002921, 002923, 002925): no driven input reaches it.
* `qh_mergecycle_neighbors` (002927): no driven input reaches it.
* `qh_mergecycle_ridges` (002929, 002931): no driven input reaches it.
* `qh_mergecycle_vneighbors` (002933): no driven input reaches it.
* `qh_mergecycle` (002995): no driven input reaches it.
* `qh_checkvertex` (003093): no driven input reaches it.
* `qh_furthestout` (003101): no driven input reaches it.
* `qh_matchduplicates` (003107, 003109): no driven input reaches it.
* `qh_printhashtable` (003133, 003135): error path only.
* `qh_setprint` (003304): error path only.

Found in passing, outside the pass: `evidence/convex-mesh-gap-2f-listing-compare.py`, `-2g-` and `-2l-` fail before
comparing (a symbol lookup that matches two candidates, and an unpacking error), with the committed
`listing_compare_lib.py` as well; they predate the pass and need their symbol names refreshed.

## Update: the testing pass

The testing pass that this hand-off asked for is recorded in `evidence/testing-pass-handoff.md`: 153 of the 184 rows were wired, driven and falsified; 31 are held with reasons.
