# Partial scene controller factory and release slice

The `NxScene::createController` and `releaseController` forwarding entries and
the tested allocation/list lifecycle slice are implemented in `NpScene.cpp` and
`Scene.cpp`. No public Physics header was changed; the SDK tree exposes
`NxController` and `NxControllerDesc` as incomplete types.

IDA evidence for the pinned DLL:

- RVA `0x0000c720` loads the internal Scene from `NpScene+0x24` and jumps to
  `Scene::createController` at `0x0005a880`.
- RVA `0x0000c730` performs the same Scene load and jumps to
  `Scene::releaseController` at `0x0005a4b0`.
- The factory rejects a nonzero descriptor word at `+8`, allocates `0x4c`
  bytes, constructs the controller and links its embedded object at allocation
  `+8` into `Scene+0x5a8`; the list link is at embedded-object `+0x30`.
- The constructor creates one kinematic box actor. Its half-extents are the
  descriptor dimensions at `+0x30`, `+0x34`, and `+0x38`, each multiplied by
  `1.1`; its position is copied from `+0x0c`, `+0x10`, and `+0x14`.
- Release unlinks the embedded controller object and calls its scalar deleting
  destructor. The oracle leaves the generated actor in the scene, so the
  candidate preserves that observed behavior.

The paired simulation regression covers an unsupported descriptor, two
controllers in one scene, actor count changes, generated actor position and box
dimensions, middle-node removal, and final head removal. On the final build,
`NxPhysicsSimulationTests` exits zero on both DLL pairs with `stdout_delta=0`
and exact stderr. Phase 5 passes 2,042/2,042 assertions; Phase 7 passes
1,338/1,338 assertions. The all-scene Viewer selection passes 48/48 selected
cases, including 43 passes and five existing signature-verified skips.

IDA confirms the primary vtable at `0x10108824` has three entries: deleting
destructor (`0x1005a470`), `move` (`0x10059710`), and `getPosition`
(`0x1005a870`). The getter returns `this + 0x28`; the constructor initializes
that vector from descriptor `+0x0c`. A private test-only ABI mirror now calls
the getter on two controllers and checks both bit-exact positions. Before the
implementation change the oracle returned the descriptor values while the
candidate's old destructor-only vtable returned invalid values; after the
change the simulation differential passes with `stdout_delta=0` and exact
stderr.

The vtable layout and getter are reconstructed. The `move` slot is present to
preserve dispatch layout. A new paired probe exercises collision-free motion in
an unobstructed path: a `+0.25` X displacement with `minDistance=0.001` updates the
controller getter to `0.25` and clears collision flags, while the generated
kinematic actor still reports its pre-step pose. The candidate's translation
subset matches those oracle observations exactly (`stdout_delta=0`, exact
stderr); the test output is registered in Phase 7.

General collision-aware sweep, step response, and hit callbacks remain
incomplete. A second paired fixture places an
axis-aligned static box at X=1.5: moving the controller from X=0 by +2 stops at
X=0.5 with collision flag 4. Repeating that move with active group mask zero
passes through to X=2.5 with no collision flags. A temporary mutation that
ignored the group mask failed the test (`candidate_exit=1`, `stdout_delta=2`);
the restored implementation matches the oracle exactly. This is a swept-AABB
box subset. A vertical fixture moving +Y toward an overhead box stops at Y=0.5
with collision flag 1, and catches a candidate that reports the horizontal
side flag for every axis. An enabled trigger shape remains a blocking sweep
candidate in this oracle build; moving toward it stops at X=7.5 with flag 4.
A clean-scene diagonal fixture also verifies tangential projection along the
X wall: displacement `(2,0,1)` ends at `(0.5,0,1)` with flag 4, while the
candidate before slide handling stopped at `(0.5,0,0.25)`. Initial-overlap
behavior is also partially pinned: from X=0.75 inside the wall's expanded
bounds, moving outward by -0.5 escapes to X=0.25 with no flags; moving inward
by +0.5 stays at X=0.75 with flag 4. The candidate's first version blocked
both directions, and the escape differential was red before adding nearest
face-directed escape handling. Convex/mesh query faces, general penetration
recovery, multiple-face sliding, step-up, callbacks, and the broader
transformed-shape matrix remain open. A 45-degree thin-box fixture moves at an
offset through the rotated box's empty world-AABB corner. The initial
candidate stopped two float ULPs early; the axis-projection sweep with
outward-rounded support intervals now matches the oracle bit-for-bit
(`bf50704e`, flag 4; Phase 7 coverage). This exercises a rotated-box sweep
case, not the complete transformed-shape matrix. The rest of the
constructor/destructor callees and complete controller interface remain open;
this is still partial controller support, not a claim that controller
behavior is complete.

The final-source focused paired differential is exact (`stdout_delta=0`,
`stderr_exact=True`; `build/controller-rotated-fullsat-diff.log`). Fresh Phase 5
passes 2,042/2,042 (`build/controller-rotated-box-phase5.log`), and Phase 7
passes 1,348/1,348 (`build/controller-rotated-box-phase7-final.log`). The
all-scenes Viewer gate passes all 48 selections on the rebuilt final DLL:
43 pass and the same five pinned-oracle asset failures skip by signature
(`build/controller-rotated-box-viewer-ctest.log`).

## Sphere obstacle dispatch follow-up

A diagonal controller path through the expanded corner of a sphere was added
as a paired simulation regression. The first run was RED: the pinned controller
passed through to `x=2` (`0x40000000`) with flags zero, while the candidate's
sphere world-AABB fallback stopped at `x=0.5` with flag 4. IDA's controller
query helper at RVA `0x00058ea0` dispatches collision geometry only for
internal shape types 2 (box) and 4 (triangle mesh). The candidate now applies
that type filter before the generic bounds fallback; the oracle/candidate
transcript matches exactly (`stdout_delta=0`, `stderr_exact=True`;
`build/controller-sphere-corner-green.log`).

Fresh Phase 5 passes 2,042/2,042 and Phase 7 passes 1,349/1,349. The rebuilt
Viewer selection passes all 44 selected CTest cases, covering all 39 demos;
34 scene runs pass and the five existing signature-verified oracle failures
skip (`build/controller-sphere-corner-viewer-ctest.log`). This closes only the
shape-type filter shown by the oracle. Triangle-mesh narrow-phase, generalized
overlap recovery, complete slide/step handling, callbacks, and the wider
controller lifecycle remain open. Public Physics headers were not changed.

## Triangle-mesh controller sweep follow-up

The shape-type filter exposed a bounds-only false positive for triangle meshes,
so the controller's supported triangle-mesh path now tests a translating
axis-aligned controller box against each mesh triangle with continuous SAT.
The tested mesh uses float vertices and 32-bit indices; transformed mesh
vertices are evaluated in world space. The first corner fixture went RED: the
oracle passed through an expanded world-AABB corner to `x=2`, while the
candidate stopped at `x=1`. After the triangle sweep, both DLLs pass through
with flags zero. A complementary fixture crosses the actual triangle and both
stop at `x=1`, retaining `y=z=0.2` and collision flag 4. The combined
`NxPhysicsSimulationTests` transcript is exact (`stdout_delta=0`,
`stderr_exact=True`; `build/controller-mesh-final-diff.log`).

Both mesh outcomes are asserted in the test and registered in Phase 7. The
verified sweep uses float vertices, 32-bit triangle indices, and an
axis-aligned controller box. The implementation also decodes 16-bit triangle
indices, but that encoding does not yet have a paired fixture. Other
vertex/index encodings, mesh/controller rotation, initial mesh penetration,
mesh sliding/step response, callbacks, and complete controller semantics are
still open. This is a tested mesh-sweep subset, not complete controller
reconstruction. Public Physics headers remain unchanged.

Final validation for this slice: Phase 5 passes 2,042/2,042 assertions
(`build/controller-mesh-final-phase5.log`); Phase 7 passes all 1,351/1,351
registered assertions (`build/controller-mesh-final-phase7.log`). The rebuilt
Viewer passes all 44 selected CTest cases (`build/controller-mesh-viewer-ctest.log`),
including all 39 available scene demos: 34 pass and the same five
signature-verified baseline skips remain. Viewer sound, physics step, and
physics contact checks pass. Inventory validation reports 6,338 functions,
5,138 data objects, and zero unexplained entries.

## Triangle-mesh initial-overlap follow-up

Two isolated paired scenes start the controller box centered on the mesh plane
at `(1.5, 0.2, 0.2)`. The first fixture moves `-0.5` X and the second moves
`+0.5` X. The oracle completes both moves without collision flags, ending at
X=1.0 and X=2.0 respectively. Before the fix, the candidate remained at
X=1.5 with side flag 4 for both moves (`stdout_delta=4`). Continuous SAT now
detects a starting AABB/triangle overlap and ignores that triangle for the
current sweep; the isolated scenes avoid introducing a second controller
actor as an obstacle. The paired simulation differential is exact
(`stdout_delta=0`, `stderr_exact=True`;
`build/controller-mesh-overlap-isolated-diff.log`). Phase 7 registers both
outcomes. This pins initial overlap for the tested mesh plane, not general
penetration recovery across arbitrary mesh shapes or multiple contacts.

The follow-up passes Phase 5 at 2,042/2,042 and Phase 7 at 1,353/1,353
(`build/controller-mesh-overlap-phase5.log`,
`build/controller-mesh-overlap-phase7.log`). The rebuilt Viewer passes all
44 selected tests, including the full 39-scene selection and sound, step, and
contact checks (`build/controller-mesh-overlap-viewer-ctest.log`). Inventory
validation remains clean at 6,338 functions, 5,138 data objects, and zero
unexplained records.
