# NxPhysics completion project plan from main

### Continuation — reverse strided range helper falsification (2026-10-07)

Closed Phase 5 row `phys_fn_000002` with the existing disassembly-based static
proof. Changing the candidate's reverse stride update into a forward step made
the focused test fail its visitation, callback-result, and negative-stride
checks; the restored Release target passes. Phase 5 now records 14 closed rows
and 191 reconstructed rows awaiting falsification. Evidence:
`docs/reconstruction/novodex-physics/evidence/range-iteration-000002.md` and
`docs/reconstruction/novodex-physics/evidence/phase5-controller-release-list.md`.
After merge to local `main` at `9fadd84e`, Phase 5 passes 2,244/2,244
assertions, and the Viewer selection passes 48/48 with all 39 scenes included
(43 passed, five existing pinned-oracle asset cases skipped).

### Continuation — Box hull facade accessor falsification (2026-10-07)

The approved Viewer scene sweep is already part of CTest and passed all 48
currently registered Viewer selections, covering all 39 scenes (43 passed;
five pinned-oracle asset cases skipped under their existing signatures). The
Phase 5 closure sweep then added direct oracle/candidate checks for the Box
hull vertex-count and three table getter slots. Four independent mutations
were each rejected; the clean hull differential passes 324/324 cases. Phase 5
now records 13 closed and 192 reconstructed rows still awaiting falsification.
Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-controller-release-list.md`.

### Continuation — Box identity vtable slot (2026-10-07)

Closed Phase 5 row `phys_fn_001391`, the Box identity method shared by vtable
slots 14–16. The shape-vtable oracle/candidate test checks that slot 14 returns
the constructed object address. A throwaway private-header mutation returning
null was caught (`failures=1`); the clean target returns zero failures. Phase 5
now had six mutation-closed function rows and 199 reconstructed rows awaiting
falsification. Evidence: `docs/reconstruction/novodex-physics/evidence/shape-self-001391.md`.

### Continuation — Box hull facade vertex pointer (2026-10-07)

Closed Phase 5 row `phys_fn_000955`, the `BoxHullFacade::vertices()` slot at
RVA `0x00020d30`. The shape-vtable differential compares 314 box-hull
rebuild/load cases against the pinned oracle. A temporary `mVertices + 1`
mutation produced 30 mismatches; the restored implementation returns zero.
Phase 5 now has seven mutation-closed function rows and 198 reconstructed rows
awaiting falsification. The full Phase 5 gate passed at 2,244/2,244 after the
restored build. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-controller-release-list.md`.

### Continuation — Box hull face count (2026-10-07)

Closed Phase 5 row `phys_fn_000961`, the box hull facade's face-count slot.
Changing the returned face count from six to five was caught by 44 cases in
`NxPhysicsShapeVtableTests`; the restored target passes all 314 hull cases.
Phase 5 now has eight mutation-closed function rows and 197 reconstructed rows
awaiting falsification. Evidence and clean/mutated output are recorded in
`docs/reconstruction/novodex-physics/evidence/phase5-controller-release-list.md`.

### Continuation — Box hull face-record getter (2026-10-07)

Closed Phase 5 row `phys_fn_000963`, the facade slot returning a face record by
index. Added six direct oracle/candidate pointer assertions because existing
hull support and rebuild checks bypassed this getter. The clean test passes
320/320 hull cases; a cyclic-index mutation fails all six new checks. Phase 5
now has nine mutation-closed function rows and 196 reconstructed rows awaiting
falsification. The Phase 5 oracle gate expects the expanded case count.

### Continuation — scene shape registration and standalone test sweep (2026-10-07)

Closed Phase 3 row `phys_fn_002423`: the body-creation differential checks the
scene's shape-pointer slot and 256-slot capacity growth after out-of-order
reuse and at ID `0x100`. Omitting the pointer store in a throwaway archive copy
was caught with `stdout_delta=6`; the clean mainline build returns an exact
oracle match. Phase 3 now records 62 closed and 330 deferred rows, with
527/527 gate assertions. The standalone Phase 5 and Phase 7 gates pass at
2,244/2,244 and 1,365/1,365. The full Viewer CTest selection passes 56/56,
covering all 39 available scenes; five known pinned-oracle asset cases skip on
their established signatures. Evidence: `docs/reconstruction/novodex-physics/evidence/scene-registration-002423.md`.

### Continuation — shared indexed-manager insertion (2026-10-07)

Discharged Phase 2 row `phys_fn_002417` through the passing Phase 3
`NxPhysicsBodyCreationTests` gate. The body-creation probe now checks both the
dynamic-body and shape managers after out-of-order ID reuse and across the
256-slot boundary. Oracle and candidate match exactly; independent body and
shape occupancy mutations each produce `stdout_delta=6`. Phase 2 now has 58
closed rows and 85 deferred rows. The Phase 3 coverage floor rises to 527 and
the Phase 5 floor to 2,244.
Evidence: `docs/reconstruction/novodex-physics/evidence/indexed-manager-002417.md`.

### Continuation — dynamic body auxiliary registration (2026-10-07)

Closed `phys_fn_002421`: the Scene auxiliary manager now registers each dynamic
body by its assigned ID, grows its sparse tables beyond 256 IDs, and expands the
active list when full. A staged-pair probe verifies out-of-order ID reuse and
registration through ID `0x100`; the omitted-record-pointer mutation is detected
with `stdout_delta=6`. Phase 5 now has five differential-closed rows, 200 rows
reconstructed but awaiting mutation, and no unreconstructed function rows.
Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-controller-release-list.md`.

### Continuation — actor descriptor userData and Phase 5 gate (2026-10-07)

IDA confirms the call from `Scene::createActor` to `phys_fn_000034` at `0x10011dac`.
The Phase 5 metadata differential exposed that the candidate dropped
`NxActorDescBase::userData`: the oracle returned the sentinel on `NxActor`, the
candidate returned false. The loader now copies it after scene registration,
once the body has retained its Scene link. The targeted differential is exact;
Phase 5 coverage rises to 2,238 assertions, and the closure ledger now records
four falsified rows, 200 reconstructed rows awaiting mutation, and one truly
unreconstructed row. The viewer sweep still covers all 39 available scenes.
Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-controller-release-list.md`.

### Continuation — Phase 5 actor teardown and mesh shape factory (2026-10-07)

Closed `phys_fn_000030` with the actor-root deletion mutation and `phys_fn_000032`
with the mesh-factory bookkeeping mutation. The mesh arm now increments the
scene mesh count and reserves mesh-work buffers from the internal vertex and
triangle counts; its large-mesh differential verifies capacity growth from 256
to 1,024. The Phase 5 registry includes `NxPhysicsMeshSimulationTests`; Phase 5
passes all 15 targets, Phase 7 passes all 11, and all 56 Viewer CTest selections
pass across all 39 scenes (51 pass; five pinned-oracle asset cases skip).
Inventory validation reports 6,338 functions, 5,138 data objects, and zero
unexplained; one Phase 5 row remains unreconstructed. The full Release build's
unrelated `NxPhysicsCollisionTests` target still fails to link
`nxInternalMeshBuildTopology`, while the `NxPhysics` DLL and Viewer targets
build. Public Physics headers remain unchanged.

### Continuation — reverse strided range helper (2026-10-06)

Reconstructed the 49-byte `phys_fn_000002` (`0x1030`) in
`Physics/src/RecoveredRows.cpp`. Its Phase 5 static proof checks reverse
iteration order, negative stride, zero-count return, and the final callback
result. The function is retained in the Release DLL map as
`_phys_fn_000002@16`. Inventory and work-unit rollups now record the row as
reconstructed. The fresh Phase 5 gate passes with 2,226/2,226 coverage
assertions, and the inventory/work-unit tests pass. Public headers remain
unchanged. See
`docs/reconstruction/novodex-physics/evidence/range-iteration-000002.md`.

### Continuation — 1,000-step three-body stack soak (2026-10-06)

Extended the public gravity/contact stack fixture from 120 to 1,000 blocking
simulate/check/fetch cycles, preserving every per-step actor pose and velocity
in the transcript. The pinned oracle and candidate match all 3,004 stack-related
lines exactly. The focused simulation differential and full Phase 5 and Phase 7
gates pass; each gate now requires the explicit 1,000-step stack result. This
adds long-horizon multi-body stability evidence to the existing 1,000-step
single-body soak, but does not close other stack sizes, solver modes, or the
broader M1 matrix.

### Continuation — controller release wrapper classification (2026-10-06)

Classified `phys_fn_002330` (`0x0005a4b0`) as reconstructed: the existing
`NxSceneInternal::releaseController` path matches its reentry guard, embedded
node unlink, deleting-destructor dispatch, and guard clear. The standalone
simulation differential exercises non-head then head removal and checks list
integrity/error counts. Phase 5 now has eight unreconstructed rows, 196
reconstructed rows awaiting whole-row falsification, and one falsified row.
The reentry and non-member error paths are backed by static evidence only; the
controller subsystem remains partial. See
`docs/reconstruction/novodex-physics/evidence/controller-release-wrapper.md`.

### Continuation — controller move ABI and approved Viewer scene gate (2026-10-06)

The controller test probe and reconstructed move slot now match the installed
NxCharacter six-argument `NxController::move` ABI. The default-argument
simulation differential remains exact. Direct Phase 5 and Phase 7 differentials
pass all 14 and 11 registered targets; all 48 Viewer selections pass, covering
all 39 scenes with five existing signature-verified oracle-asset skips. The
full Phase 5 gate passes at 2,225/2,225 coverage assertions. Phase 7 passes at
1,364/1,364 with external pair staging; the default pair directory under
`build` caused the pinned oracle to fault during its raycast differential.
This closes the call ABI only; successful step-over, non-default sharpness,
groups-mask filtering, and full controller behavior remain open. See
`docs/reconstruction/novodex-physics/evidence/controller-move-public-abi.md`.

### Continuation — authored tetrahedron PMap density 80 (2026-10-06)

Added an isolated density-80 compute executable and Phase 4 coverage registration.
The pinned oracle and rebuilt candidate both produce a 144,272-byte PMap with
FNV-1a `1c6814b928a39f10`; the focused paired differential passes with identical
stdout/stderr. The existing all-scene Viewer gate remains part of verification
and continues to cover all 39 available demos. Fresh Release CTest passes all
48 Viewer selections: 34 runnable scene demos and both focused Viewer physics
checks pass, while five existing oracle-asset cases skip on their verified
failure signatures. This closes only the authored tetrahedron at density 80;
other PMap topologies/resolutions and the full-DLL audit remain open. See
`docs/reconstruction/novodex-physics/evidence/pmap-resolution80.md`.

Date: 2026-09-30
Baseline: `main` at `b942f01` (Merge convex-mesh gap reconstruction into main)
Status: Active execution. The user selected full-DLL reconstruction and standalone simulation tests before Unreal integration.

### Continuation — grounded controller sweep at floor contact (2026-10-06)

Added a fresh-scene differential where the controller begins exactly at floor
contact, moves down slightly while sweeping toward a low wall, and must classify
the time-zero floor contact on the vertical axis. The oracle advances to
`(0.5, 0.5, 0)` with flags `0x5`; candidate previously remained at the start
with flags `0x4`. Preserving the inward zero-time boundary axis makes the
simulation transcript exact. Phase 5 passes 2,042/2,042; Phase 7 passes
1,358/1,358. Successful step-over and general penetration recovery remain
open. The full Release Viewer selection passes 48/48, with all 39 scenes
represented, 43 passes, and five signature-verified oracle asset skips. See
`docs/reconstruction/novodex-physics/evidence/controller-grounded-sweep.md`.

### Continuation — controller step-offset enable-byte gating (2026-10-06)

The approved isolated grounded low-obstacle fixture now sets `stepOffset` to
zero while keeping descriptor `+0x1c` nonzero. The oracle still stops at
`(0.5, 0.5, 0)` with flags `0x5`; the candidate was RED at flags `0x6` because
its private probe-enable byte incorrectly also required a positive step offset.
IDA shows the constructor copies only `descriptor[7] != 0` into object byte
`+0x34`. The candidate now preserves that field independently of
`descriptor+0x2c`, and the complete simulation differential is exact
(`stdout_delta=0`, `stderr_exact=True`; `build/controller-step-offset-green.log`).
Phase 7 passes 1,358/1,358 assertions; Phase 5 passes 2,042/2,042; the Viewer
physics step/contact selection passes. The positive-step-offset fixture
remains covered separately. Successful step-over motion and broader controller
semantics remain open. See
`docs/reconstruction/novodex-physics/evidence/controller-step-offset-gating.md`.

### Continuation — public 16-bit mesh input in controller sweep (2026-10-06)

Added a paired controller face-hit fixture using a triangle mesh cooked from
`NX_MF_16_BIT_INDICES`. Both DLLs stop at `x=1, y=z=0.2` with collision flag
4, and expose the cooked indices as `NX_FORMAT_INT` with a 12-byte stride.
This covers the public descriptor-to-controller path; the private
`NX_FORMAT_SHORT` decode branch remains unreachable through the current
`TriangleMesh` format. The focused differential is exact (`stdout_delta=0`,
`stderr_exact=True`), and Phase 7 registers the new output at a 1,356 assertion
floor. The fresh Phase 7 gate passes all 11 registered differentials with
1,356/1,356 assertions. Other mesh formats, transformed controllers, successful
step-over behavior, callbacks,
and complete controller semantics remain open. See
`docs/reconstruction/novodex-physics/evidence/scene-controller-factory.md`.

### Continuation — SDK indexed scene lookup (2026-10-06)

Completed the previously blocked `NxPhysicsSDK::getScene` wrapper slot using
the existing measured `Scene+0x6cc` wrapper field. The paired SDK test creates
two scenes, verifies both indexed wrappers, releases the first and verifies
the survivor moved to index 0, then releases the survivor. Returning null was
RED (`stdout_delta=4`); the reconstructed wrapper lookup is GREEN. See
`docs/reconstruction/novodex-physics/evidence/sdk-get-scene.md`. This closes
valid-index lookup only; the oracle's out-of-range fault is recorded from
disassembly and is not exercised in-process. Full-DLL reconstruction remains
open. The fresh Phase 2 gate passes (including `NxPhysicsInternalTests`), and
the all-scene Viewer selection passes 48/48 registered cases: all 39 available
scenes are represented, with 43 passes and five existing signature-verified
asset skips. Public Physics headers remain unchanged.

### Continuation — controller mesh back-face sweep (2026-10-06)

Added an isolated scene using the same triangle geometry with reversed winding.
The oracle ignores motion into the triangle's back face and reaches `x=2.0`
with no collision flags; the candidate initially stopped at `x=1.3` with flag
4. The controller mesh sweep now rejects back-face motion after checking the
initial-overlap case, and the paired transcript matches exactly. Phase 5 passes
2,042/2,042 assertions; Phase 7 passes at 1,354/1,354; the all-scene Viewer
selection passes 48/48, covering all 39 demos with the same five
signature-verified skips. This closes the tested mesh-sidedness case only;
step-up and the broader controller behavior remain open. Public Physics
headers remain unchanged. See
`docs/reconstruction/novodex-physics/evidence/scene-controller-factory.md`.

### Continuation — all-scene Viewer startup and gate (2026-10-06)

The first serial Viewer sweep showed every scene launch spending 79–98 seconds
in GLFW's Win32 keyboard-layout scan, before Viewer used any input. The live
stack was in `ToUnicode` under `_glfwUpdateKeyNamesWin32`. Hidden Viewer tests
now opt out of that unused key-name table scan through a test-only environment
variable; interactive Viewer startup retains the normal path. The complete
Viewer selection passes 48/48 registered cases (43 passed, five existing
signature-verified oracle-asset skips), including all 39 available scene demos
and both focused physics checks (`ctest --test-dir build -C Debug -R '^Viewer'
--output-on-failure`, 55.21 sec). This closes the current Viewer gate only;
M1 and full-DLL reconstruction remain active. Public Physics headers are
unchanged.

### Continuation — controller sphere obstacle dispatch (2026-10-06)

Added a paired controller move through the expanded AABB corner of a sphere.
It first went RED: the oracle passes through to `x=2` with no flags, while the
candidate's generic shape-bounds fallback stopped at `x=0.5` with flag 4. IDA's
resolver at RVA `0x00058ea0` dispatches candidate geometry only for internal
shape types 2 (box) and 4 (triangle mesh); the candidate now applies that type
filter. The differential is exact (`stdout_delta=0`, `stderr_exact=True`;
`build/controller-sphere-corner-green.log`). Fresh Phase 5 passes 2,042/2,042,
Phase 7 passes 1,349/1,349, inventory validation reports 6,338 functions /
5,138 data objects / zero unexplained rows, and the full Viewer selection passes
all 44 selected cases across all 39 demos (34 scene passes and five existing
signature-verified skips). This is partial controller geometry only;
triangle-mesh narrow-phase, general penetration recovery, complete slide/step
response, callbacks, and the rest of the controller matrix remain open. See
`docs/reconstruction/novodex-physics/evidence/scene-controller-factory.md`.

### Continuation — controller free-space move slice (2026-10-06)

Added a paired call through the recovered second controller vtable slot. The
oracle advances the controller's exposed position by a clear-path `+0.25` X
displacement and clears the collision flags; the generated actor still reports
its pre-step pose. The candidate's collision-free translation subset matches
exactly, and the two oracle outputs are now pinned in Phase 7. Phase 5 passes
2,042/2,042; Phase 7 passes 1,340/1,340; the rebuilt Viewer selection passes
48/48, with five existing signature-verified scene skips. Collision sweeps,
active-group filtering, slide/step response, and hit callbacks remain open, so
this is partial controller movement only. See
`docs/reconstruction/novodex-physics/evidence/scene-controller-factory.md`.

### Continuation — controller axis-aligned obstacle sweep (2026-10-06)

Added a public-scene obstacle differential around the private controller ABI.
The pinned DLL stops at X=0.5 and reports collision flag 4 when a half-meter
controller moves +2 toward a static box at X=1.5. With active group mask zero,
the same controller passes through to X=2.5 with no flags. The candidate now
queries the scene's selected pruner over the swept controller AABB, filters
candidate groups and the controller's own actor, and clips the translation at
the earliest box-AABB intersection. The initial test was red (`stdout_delta=2`,
candidate X=2/flags=0); after implementation the standalone differential is
exact (`stdout_delta=0`, `stderr_exact=True`). A mutation removing group-mask
filtering is caught. Phase 7 coverage now pins both new outputs. This does not
reconstruct oriented/mesh sweeps, initial overlap, slide/step response, hit
callbacks, or the remaining controller interface.

Follow-up vertical and trigger fixtures were then added. The oracle clips a
controller moving +Y toward an overhead static box at Y=0.5 and reports flag 1;
the initial candidate had the right position but the wrong axis flag, and the
test caught the bug. The fix retains the entering slab axis. The oracle also
blocks on the tested trigger shape at X=7.5 with flag 4. The simulation
differential is exact for free movement, horizontal and vertical obstacles,
active-group exclusion, and this trigger case. Final relevant checks: Phase 5
2,042/2,042; Phase 7 1,344/1,344; all 48 Viewer selections pass, with five
existing signature-verified scene skips. These checks cover smoke loading and
the registered physics probes; they do not close the full-DLL goal.

### Continuation — controller tangential wall slide (2026-10-06)

A clean-scene diagonal fixture makes the unresolved slide behavior observable:
the oracle moves `(2,0,1)` into an X wall, stops at X=0.5, continues along the
wall to Z=1, and reports flag 4. The previous candidate stopped the tangential
component at Z=0.25 (`stdout_delta=2`); the candidate now projects the remaining
motion off the contacted axis and continues up to four axis-aligned contacts.
The focused oracle differential is exact (`stdout_delta=0`, exact stderr).
Phase 5 passes 2,042/2,042, Phase 7 passes 1,345/1,345, and the full Viewer
selection passes 48/48 with five existing signature-verified scene skips.
The Phase 7 target and coverage floor have been extended by one. This remains
a bounded AABB implementation: arbitrary convex/mesh sweeps, initial
penetration, full step behavior, and callback semantics remain open.

### Continuation — initial-overlap escape direction (2026-10-06)

Added a clean-scene pair with the controller at X=0.75 inside the expanded
static-box bounds. The oracle allows motion outward by -0.5 to X=0.25 with no
collision flag, but blocks motion inward by +0.5 at X=0.75 with flag 4. The
first candidate stopped both directions (`stdout_delta=2`); it now identifies
the nearest expanded-box face and permits motion directed toward that exit.
The paired simulation differential matches exactly for both directions. This
is bounded directional recovery for axis-aligned boxes and does not reconstruct
general depenetration or alternate contact manifolds. Phase 5 passes
2,042/2,042, Phase 7 passes 1,347/1,347, and the complete Viewer selection
passes 48/48 with five existing signature-verified oracle skips.

### Continuation — rotated controller box sweep case (2026-10-06)

Added an oracle fixture that sweeps past the empty corner of a 45-degree thin
box. It first exposed a two-ULP contact-position difference between the
axis-aligned candidate bound and the oracle. The controller now performs a
15-axis swept AABB/OBB interval test for rotated box obstacles, with outward
rounding on the support radius; the full `NxPhysicsSimulationTests` differential
passes exactly (`stdout_delta=0`, `stderr_exact=True`). The result is pinned in
Phase 7, whose fresh final-source gate passes 1,348/1,348 assertions; Phase 5
also passes 2,042/2,042. The rebuilt Viewer passes all 48 selected cases: 43
pass and five verified oracle-asset cases skip by signature, including all 39
available scenes and both focused Viewer physics checks. The broader
transformed shape matrix, convex/mesh sweeps, penetration recovery, full
slide/step handling, and hit callbacks remain open. No public Physics headers
changed.

### Continuation — controller position ABI (2026-10-06)

The approved all-scenes Viewer selection remains part of verification. A new
test-only private ABI probe calls the recovered controller position getter.
The first differential was RED: the oracle returned the two descriptor
positions, while the candidate's destructor-only vtable produced invalid
values. IDA shows the three-slot primary vtable (deleting destructor, move,
getPosition), with getPosition returning `this + 0x28`. The candidate now
installs those slots, initializes object position from descriptor `+0x0c`, and
returns that state through getPosition. `NxPhysicsSimulationTests` is exact
after the fix. Fresh Phase 5 and Phase 7 gates pass at 2,042/2,042 and
1,338/1,338 assertions. The approved Viewer selection passes all 48 tests:
43 pass, including the runnable scenes and both focused Viewer physics cases;
five pinned-oracle asset failures retain their signature-checked skip status.
The collision-aware move algorithm remains open; this is still partial
controller support. No public Physics header changed.

### Continuation — scene controller factory (2026-10-06)

Recovered the `NpScene` forwarding entries and implemented a tested slice of
the Scene-owned controller allocation/list lifecycle. The first paired regression exposed the
factory's generated kinematic box actor; the strengthened case now verifies its
descriptor-derived position and dimensions, rejects an unsupported descriptor,
and removes two controllers from the middle/head of the Scene list. The
standalone simulation differential is exact. Current Phase 5 passes
2,042/2,042 assertions; Phase 7 passes 1,338/1,338 assertions. The all-scene
Viewer selection passes 48 selected cases (43 passed, five existing
signature-verified skips). See
`docs/reconstruction/novodex-physics/evidence/scene-controller-factory.md`.

This remains an explicitly partial slice: the primary vtable layout and
position getter are recovered, while collision-aware movement and the rest of
the constructor/destructor callees remain open. Full-DLL completion and Unreal
integration remain open.

### Continuation — spring/damper scene-step integration (2026-10-06)

Closed a missing scene-step dispatch edge: `NxSceneInternal::simulateFrame`
walks effectors after island contact solving and calls slot 2 (`Effector::tick`)
before post-step velocity bookkeeping. A one-dynamic-body/world-anchor fixture
was RED before the change (the second-step oracle x velocity was `0x3e8e38e3`,
while candidate stayed zero) and is now an exact paired differential.
Verification: Phase 5 passes 2,042/2,042; Phase 7 passes 1,326/1,326; the
approved all-scene Viewer selection passes all 48 selected cases, including 34
scene runs and five existing signature-verified oracle skips. See
`docs/reconstruction/novodex-physics/evidence/effector-step-integration.md`.

This fixture is partial dynamic evidence for `phys_fn_000655`; the source now
translates the full recovered row, so the inventory marks it reconstructed.
Whole-row mutation falsification remains open. Public Physics headers remain
unchanged.

### Continuation — disabled FluidManager scene-step calls (2026-10-06)

Oracle `phys_fn_000659` calls the disabled FluidManager step hook after every
substep and surface-mesh generation once after the substep loop. The scheduler
had omitted both calls. A public scene fixture with the manager's conditional
extension-dirty byte pinned to zero was RED (three oracle warnings, zero
candidate warnings) and is now an exact differential. Phase 5 passes
2,042/2,042, Phase 7 passes 1,327/1,327, and the approved all-scenes Viewer
selection passes 48/48, with five existing signature-verified skips. See
`docs/reconstruction/novodex-physics/evidence/fluid-manager-scene-step.md`.

Inventory rows 000655 and 000659 now record their reconstructed scheduler/step
control flow; mutation falsification remains open. Rows 003630 and 003632 only
have partial disabled-empty-manager behavior. Extension-backed static
collision, live-fluid updates, and surface generation remain open full-DLL work.

## Outcome and constraints

### Continuation — current-build DemoGame smoke (2026-10-06)

Reran `DemoGame.exe PhysTest` against isolated oracle and candidate pairs after
the scheduler change. Both processes exit 0, load `PhysTest`, initialize and
shut down cleanly. Module polling confirms each process loaded Physics and
Foundation from its own staged `Binaries`; the same four `NpActor.cpp:916`
center-of-mass diagnostics appear on both sides. Logs and binary hashes are
recorded in
`docs/reconstruction/novodex-physics/evidence/demogame-phystest-smoke-2026-10-06.md`.
This revalidates the one-map T2 smoke against the current DLL; the broader
Unreal interaction matrix and full reconstruction remain open.

Finish reconstructing the complete pinned Win32 NxPhysics.dll, including functionality Unreal rarely uses. Build it and NxFoundation.dll through the existing CMake project. Preserve all public Physics headers and the Foundation ABI. Product code must be independent of the original DLL; the original is an oracle for analysis and separate-process tests only.

The user selected **standalone simulation tests first, then Unreal**. Begin useful testing as soon as the real simulation path works. Early testing is an intermediate milestone; it does not reduce the full-DLL scope or authorize declaring unfinished subsystems complete.

This roadmap supersedes the scheduling assumptions of the September 24 completion plan. Existing contracts, recovered source, oracle listings, and proofs remain inputs. Do not repeat already completed NpActor, joint, scene-raycast, qhull, or convex/mesh reconstruction just because an older checklist is unchecked.

## 1. Verified baseline

The assessment used the current source and inventory on main, not the older `codex/nxphysics-completion` worktree. Untracked user files were left alone.

| Item | Current evidence | Planning consequence |
|---|---|---|
| CMake Win32 Release DLL build | Fresh invocation of `cmake --build build --config Release --target NxPhysics` succeeds; existing build cache uses Visual Studio 18 2026 / Win32 | Preserve the working build; avoid build-system replacement |
| Public Physics headers | Manifest verification passes for all 80 files | Keep the hash check mandatory |
| Phase 5 | The original baseline passed 13 staged-pair targets (2,037/2,037 assertions), while the object-layout target emitted `CANDIDATE-MISSING family=vtables`. That marker has since been replaced with executable checks; the fresh Phase 5 gate now passes at 2,037/2,037. | Keep Phase 5 green while continuing the broader vtable, ownership, and whole-DLL audit. The gate repair is not evidence that all actor virtuals or every object family is complete. |
| Inventory validation | Pass; 6,338 function records, 5,138 data records, zero unexplained rows | This proves ledger consistency, not completion |
| Executable code census | 2,787 code rows / 938,498 bytes: 749 discovered, 1,887 reconstructed, 145 dynamically gated, 6 statically reviewed | Separate missing implementation from verification debt |
| Discovered code | 218,282 bytes across 749 unique IDs | This is an audit queue, not a claim that every byte is unwritten |
| Data | All 5,138 records are classified | Prove candidate ownership and relocation for required tables/globals; classification alone is insufficient |
| Work-unit map | Current `work_units.json` has 109 units (60 named, 49 gaps). The generator reproduces the same row ownership, and the committed-map test verifies unique names plus exactly one owner for each of 2,787 executable rows. The map's state/byte rollups lag the current inventory and are being refreshed. | Use the verified ownership map for scheduling; keep its generated rollups synchronized as inventory states change |
| Scene simulation API | `getGravity`/`setGravity`, `getTiming`/`setTiming`, and the write-lock `isWritable` probe match oracle outputs; `startRun`/`finishRun`/`runFor` follow the oracle's deprecated-warning and call sequences. `simulate`, `checkResults`, `fetchResults`, and fence APIs remain open. | The worker/event lifecycle and real stepper are the immediate blockers to useful physics simulation tests |
| Final gate | Phase 8 has no registered test targets and coverage floor zero | A separate whole-DLL acceptance gate must be built |

The original current-main artifacts are `build/main-phase5.log` and `build/main-phase7-with-simulation.log`; they record the pre-repair state. Subsequent work added the real public scene step/result path, so the old `stdout_delta=30` stationary-candidate failure is superseded by the checkpoint below. The explicit-friction differential was the last Phase 7 failure and now passes on the current worktree revision; the test-matrix and full-DLL work remain open. Other phase results should be tied to their current logs and source revision. Older branch reports have different coverage floors and must not be presented as current-main verification.

### Execution checkpoint — 2026-09-30

- The explicit Phase 5 red marker has been removed after the object-layout gate gained executable candidate checks. The fresh gate against this worktree passes with 2,037/2,037 recorded assertions (`build/phase5-worktree-final.log`). This closes the stale red gate, while the roadmap's broader actor/shape vtable and whole-DLL audits remain open.
- The real standalone scene step now runs the existing collision/contact path and settles the dynamic sphere on the static plane. The 1,000-step result matches the oracle exactly at `y=0.45`, zero velocity.
- A two-dynamic-sphere impact fixture now reaches the public simulate/check/fetch path through contact generation and the kind-0 support solver row. After correcting the x87 precision boundary from the oracle listing, the complete fixture output matches (`stdout_delta=0`). Its first impact frame is pinned in Phase 7 coverage.
- A no-gravity conventional-force fixture now reaches the same public step path and matches the oracle exactly after one 0.125-second step (`position.x=3d747645`, `velocity.x=3ef47645`). Its result line is registered in Phase 7 coverage.
- The worker path now covers both nonblocking outcomes: before any submitted work, `checkResults(false)` and `fetchResults(false)` return false; after blocking until an empty step completes, both nonblocking calls return true. Oracle and candidate outputs match exactly, and both result lines are registered in Phase 7 coverage.
- A kinematic body driven with `moveGlobalPosition` reaches its target through the real variable-step path and has zero velocity after the step, matching the oracle. A stationary body also expires its wake counter and sleeps after 30 fixed steps, then wakes through `wakeUp(0.5)`; both states match the oracle.
- The explicit-friction sphere/plane probe now matches the oracle through all eight steps. The first remaining issue was the solver-to-patch callback: candidate pre-divided the impulse and supplied the iteration number where the oracle supplied the timestep, adding an unintended float rounding. After matching that call contract, the normal and friction support rows now scale the signed impulse by inverse mass before projecting along the constraint direction, matching the oracle's x87 operation order.
- A three-body stack regression now runs 120 fixed steps through the public scene/contact/solver path. Its first divergence had been at step 4: the oracle reduced the gravity increment for bodies with multiple contact pairs, while candidate `BodyStep.cpp` read an unrelated hard-coded zero. An oracle debugger write watch showed SDK initialization copies the live parameter defaults into that data region, and the corresponding adaptive-force parameter is `1.0` in the running oracle. `row000726` now reads the live SDK parameter instead of a detached zero; the full simulation fixture matches exactly (`stdout_delta=0`, `stderr_exact=True`) in `build/stack-fixed-differential.log`.
- After the stack fix, the fresh Phase 7 gate passed all eight targets at 1,165/1,165 assertions (`build/phase7-stack-fixed.log`), Phase 5 passed 2,037/2,037 (`build/phase5-after-stack-fix.log`), the CMake Release build succeeded, and the reconstruction tooling suite passed all 770 tests. The current fixed-joint follow-up also passes fresh Phase 5 and Phase 7 runs, and the full standalone simulation target matches. No public Physics headers changed. This closes the current stack mismatch, not the full M1 matrix. Repeated scene/SDK lifecycle cases are still outstanding. Solver kinds 1/2/3/6 now have a provisional 004397 path; kind 5's 004399 and broader per-kind fixture coverage remain open. Full-DLL completion remains unchanged.
- The fixed-joint probe initially exposed a scene lifecycle defect: `Scene::simulate()` omitted the second per-joint `row000720` pass and did not reset the used-joint array end (`Scene+0x590`) to its start (`Scene+0x58c`). On the next fixed frame, `row000762` linked the joint to itself, and a worker spun while traversing that list. Both operations are now restored from oracle row `000635`, and all 12 fixed steps complete. Raw oracle disassembly shows `FixedJoint::row_slot6` does not set `mFlags` bit 2; the speculative write was removed. The trial kind-1/2/3/6 handler also rounded the `004397` impulse too early. Moving the float store to the recovered accumulated-impulse boundary made the complete standalone simulation differential green again (`stdout_delta=0`, `stderr_exact=True`; `build/fixed-joint-retest2.log`). This validates the fixed-joint fixture and its tested kind-3 route, not every solver kind or the full M1 matrix; keep the handler marked provisional until broader fixtures cover kinds 1, 2, and 6.
- The fresh Phase 7 run after this change passes all eight targets (`build/phase7-after-fixed-gate-serial.log`); Phase 5 passes all 13 (`build/phase5-after-fixed-gate-serial.log`). The 770-test tooling suite passes in 212.8 seconds. These results begin T1's requested standalone testing and keep Phase 5 green; repeated lifecycle cases and kinds 1/2/6 coverage remain open.
- The initial T2 launcher probe was superseded by the user's follow-up that DemoGame now works outside the reconstruction worktree. This establishes a usable installed-engine baseline; loading the rebuilt candidate pair and running the same scenario remain open. Full-DLL completion remains unchanged.
- The default `001976` broadphase/contact refresh is now wired into each real simulation substep. Its static-plane settle, explicit-friction, 120-step three-body stack, and 40-step two-body impact fixtures match the pinned oracle exactly. Fresh checks on this worktree pass the Release build, Phase 3 (2 targets), Phase 5 (13), Phase 6 (6), and Phase 7 (8); the standalone simulation target has zero stdout delta and exact stderr. The full tooling suite passes 770 tests plus 690 subtests; inventory validation reports 6,338 functions / 5,138 data objects / zero unexplained; all 80 public headers match the manifest. Phase 4 currently has no registered test targets, so its runner skips it. These results verify the tested mode-0/default-pruner route only: other pair-refresh selectors, the full joint solver dispatch, repeated lifecycle, callbacks, and mesh contact remain open.
- Mode 3 now uses a persistent `Opcode::SweepAndPrune` coherent cache. Oracle evidence in `ContactPlaneMesh.cpp__to__PenetrationMap.cpp.md` identifies the initialization, changed-box update, and pair-enumeration paths; pruning-engine add/remove and destruction invalidate or release the cached tree. A public simulation fixture moves one actor out of a three-body overlap chain and back in. Oracle and candidate pair counts are one and then two, and the full transcript matches (`stdout_delta=0`, `stderr_exact=True`) in `build/coherent-sap-differential.log`. Both lines are registered in Phase 7; its focused coverage-floor tests pass and fresh Phase 7 passes at 1,192/1,192 (`build/phase7-coherent-sap.log`).
- The coherent-cache result covers only the tested add/remove transitions. Bounded-scene behavior, multi-object update ordering, mode-1 traversal fidelity, pair callback/report lifecycle, and broader invalidation cases remain open. T1 and full-DLL completion remain open.
- The three selectors now also pass a dense four-body pair-order case (`0-1,0-2,0-3,1-2,1-3,2-3`) and bounded-scene contact coverage. Oracle and candidate both report two bounded-scene pairs for each selector. A red differential exposed dynamic actors being registered into hard-coded pool 2 when the bounded descriptor selects pool 1; registration now follows the engine's descriptor-selected pool and creates its 0x40-byte bounded dynamic pruner. The Phase 7 floor is 1,201 and the fresh gate passes. Bounded-pruner tree-backed query slots are still incomplete, so this closes only the exercised simulation registration/contact route.
- The `004397` joint solver callback now matches the oracle vtable dispatch (slot 3 for final accumulated-force updates), and its force accumulator uses the separate ±maxForce clamp and slot-2 break check shown in the listing. The distinct kind-5 `004399` solver has been transcribed and wired for both iterative and final-record passes; inventory state is `reconstructed` on static listing evidence, with dynamic kind-5 coverage still open. The 12-step kind-1 distance-joint fixture exposed a linear impulse rounding-order mismatch; `supportApplyJointImpulse004395` now rounds the projected impulse before inverse-mass multiplication, following the oracle listing. After a fresh CMake rebuild, all 12 distance states match exactly and the fixture is registered in Phase 7. Fresh Phase 5 (13 targets) and Phase 7 (8 targets) pass; the simulation target has `stdout_delta=0`, and the Phase 7 floor is 1,214. Focused coverage registry checks pass. The low-break-force fixed-joint response and broader per-kind matrix remain open, so this does not close M1.
- On 2026-10-01, fresh Phase 5 (13 targets), Phase 6, and Phase 7 (8 targets) pass (`build/phase5-after-probe-removal.log`, `build/phase6-after-probe-removal.log`, `build/phase7-final-current.log`); the registered simulation target is exact (`stdout_delta=0`, `stderr_exact=True`). The four-step low-force fixed-joint probe still produces `stdout_delta=6`; it stays outside the shared gate. Oracle disassembly confirms 004395 is a record-thiscall with impulse and timestep stack arguments (`ret 8`); the candidate uses that ABI and reverse-column inertia accumulation. An attempted change to keep angular matrix sums unrounded increased the mismatch to 28 lines and was reverted. A separate 12-step revolute pendulum probe produced `stdout_delta=20` (`build/revolute-sim-differential.log`). A fresh all-kind trace found 384 matching solver-row entries and matching post-helper body states; all 77 traced slot-3 accumulated-force callback outputs also match, including row accumulators, body linear/angular velocity, and the joint accumulator at +0x154. Those traces were preliminary; the subsequent current-worktree trace isolated a missing solver-wrapper callback.
- The coherent-broadphase moved-apart/rejoin regression then exposed a kind-0 contact row rounding mismatch in body 2's X velocity. Oracle and candidate row inputs matched, but `phys_fn_004403` projects the signed impulse before applying inverse mass for body 2. `JointSupport.cpp` now preserves that operation order, and the regression reads the internal body record so it cannot pass through a public getter that returns stale state. The exact expected velocity bits match after the fix. Fresh Phase 5 and 6 gates pass at 2,037/2,037 and 856/856 (`build/phase5-after-contact-fix.log`, `build/phase6-after-contact-fix.log`); Phase 7 passes all eight targets at 1,218/1,218 with zero differential output (`build/phase7-after-contact-fix.log`). The full tooling suite then passed 770 tests in 214.2 seconds; header verification passed all 80 files, inventory validation passed with 6,338 functions, 5,138 data objects, and zero unexplained records, and `git diff --check` passed. The low-force fixed-joint response (`stdout_delta=6`) remains an unresolved standalone case; the revolute mismatch is resolved below. Public Physics headers remain untouched.
- The 12-step public-API revolute fixture reproduced the former `stdout_delta=20` mismatch. Current paired debugger traces matched through ordinary solver rows and localized the first difference to final-pass kind 6 handling: oracle `004174` clears the row target and dispatches joint vtable slot 1 before the kind 6 solve, while the candidate skipped that reset. `JointSupport.cpp` now performs the slot 1 callback at the corresponding point. The fixture now matches across all 12 steps (`stdout_delta=0`, `stderr_exact=True`); fresh Phase 6 and Phase 7 gates pass at 856/856 and 1,218/1,218 (`build/gate-phase6-kind6-reset.log`, `build/gate-phase7-kind6-reset.log`). Full solver-kind interaction coverage, the low-force fixed-joint case, and full-DLL completion remain open.
- The low-force fixed-joint mismatch is now resolved and registered in the Phase 7 simulation corpus. Oracle tracing identified the missing 000577 break-event queue drain in fetchResults; the candidate now dispatches 004113, applies the 004105 remove-before-clear path, and then completes finishSimulation in oracle order. The focused fixture and all eight Phase 7 targets pass with exact output, and all 13 Phase 5 targets remain green (`build/break-event-exact-detach.log`, `build/phase7-break-event-drain-registered.log`, `build/phase5-after-break-fix.log`). The nine registered break observations raise the Phase 7 floor to 1,227. This closes the tested default-notify break route, not all joint break callback outcomes or the broader solver-kind matrix. The regenerated scheduling map at `build/current-work-units.json` has 109 units (60 named, 49 gaps). Remaining high-priority clusters include OPCODE mesh interface (58 rows/40,204 bytes), Controller-to-Fluid (91/29,058), PlaneMesh-to-PenetrationMap (43/20,213), TriangleMesh-to-Controller (24/16,739), and Fluid-to-FluidManager (46/14,051). Continue M1 with solver-kind interactions and repeated lifecycle, then close the triangle-mesh object/load prerequisites before claiming mesh-contact coverage. Public Physics headers remain untouched.
- A temporary public-API D6 swing-limit probe confirms kind 2 reaches the solver. Comparing the same four-step 60-degree swing fixture showed that candidate and oracle positions and quaternions match bit-for-bit; only tiny angular-velocity values differ (six stdout bit-field differences). The initial solver-row inputs match, and tracing narrowed the residual to angular-impulse cancellation around the final solver pass, but did not yet prove the exact rounding source. The probe and logging instrumentation were removed without registering the unresolved case in Phase 7. Revisit with a focused `004395` angular-impulse differential before claiming kind-2 response closure (`build/d6-diag-diff4.log`, `build/d6-kind2-oracle-all-cdb.log`).
- On 2026-10-03, the formerly red `NxTriangleMeshShapeDesc` actor probe was traced to `nxActorShapeFactory` omitting type 4, with no installed MESH family table or public mesh handle. The factory now builds the 0xe8 runtime shape, dispatches descriptor load and owner update through the MESH table, exposes type 4 and the original public mesh through the shape handle, and dispatches mesh slot 0 during actor teardown. A differential probe first confirmed actor creation, then a second probe caught and fixed the wrapper offset in `getTriangleMesh`. The registered `NxPhysicsSimulationTests` target now matches exactly; Phase 7 passes at 1,300/1,300 and Phase 5 at 2,037/2,037, inventory validation passes with 6,338 functions / 5,138 data objects / zero unexplained, and the CMake Release build succeeds. This closes the exercised single-mesh actor create/readback/release route only: `phys_fn_000032` remains partial, compound construction, remaining MESH virtuals, mesh model/tree construction, mesh contact, and full Phase 5 closure remain open. No public Physics headers changed.
- The actor's `getWorldBounds` probe showed the new mesh path returning a zero box. Oracle evidence pins `TriangleMesh+0x44..+0x58` as local min xyz/max xyz; the constructor initializes it to the empty-box sentinels, and MESH slot 9 consumes it directly when no support tree exists. The private layout now names those six floats and ordinary mesh loading accumulates the input vertex bounds. The fresh `NxPhysicsSimulationTests` differential matches exactly (`stdout_delta=0`, stderr exact), and the Win32 Release `NxPhysics` target builds. This is evidence for the ordinary loaded-mesh/no-tree bounds path only; `phys_fn_002260` mesh-model construction, tree-backed bounds, serialization completeness, mesh collision, and the broader TriangleMesh unit remain open. No public Physics headers changed.
- On 2026-10-03, the registered mesh simulation now queries the built OPCODE model for sphere candidates, and ordinary sphere/mesh contact rejects an overlapping sphere on the triangle's back side. The regression first showed seven candidate callbacks against zero in the oracle; after the face-side filter, the drop and underside cases match bit-for-bit. Matrix-B `NxOverlapSphereMesh` is also wired from the `001925` listing and reaches a public mesh-trigger enter callback; the fixture rotates and translates the trigger mesh and matches the oracle. A second trigger fixture puts a sphere inside the mesh bounds but outside both triangles; the oracle and candidate both report exactly one expected enter, with no unexpected callback. A boundary-hit fixture places the sphere center outside the mesh footprint; its reported contact point is now the oracle's closest mesh point `(2,0,0)` and its separation matches bit-for-bit after using the stored local delta and x87 operation order. A separate 90-degree rotation plus translation fixture matches the world-space contact point at `(-10,5,0)`. The focused differential and fresh Phase 7 gate pass at 1,305/1,305 (`build/phase7-mesh-edge-separation.log`); the mesh target has `stdout_delta=0` and exact stderr. Boundary normal and post-solver trajectory still need exact comparison and reconstruction. Both installed Physics public-header manifests pass (80 files). This advances the ordinary sphere/mesh paths, not full semantic closure of `001927`/`001929`: heightfield-specific feature normals, the full edge/corner branches, broader transformed/multi-face contact coverage, solver response for the boundary case, and mutation evidence remain open. Inventory closure evidence also remains open. No public Physics headers changed.

### Execution checkpoint — 2026-10-03, sphere-mesh normal and Phase 5 repair

- The new normal observations localized the boundary differential to precision loss: face and edge contact points, separations and post-step actor state already matched, while patch normals differed by one or two ULPs. `NxContactSphereMesh` now keeps the `NxPointTriangleSquareDistance` double squared-distance through x87 `sqrt`, reciprocal and per-component multiplies. The two face patches and the boundary-edge patch now match all oracle normal words, and the fixture also pins patch counts, per-patch normals, separation, position and velocity.
- Fresh `NxPhysicsMeshSimulationTests` differential passes with `stdout_delta=0` and exact stderr. Phase 7 passes at 1,305/1,305 (`build/phase7-mesh-normal.log`).
- Phase 5's fresh build exposed missing `NxContactSphereMesh` and `NxOverlapSphereMesh` symbols in the static object-layout/vtable harness. Added link-only definitions to `tests/PhysicsInternalContactStubs.cpp`; production and simulation targets continue linking the real sphere/mesh implementations. Phase 5 now passes all 13 targets at 2,037/2,037 (`build/phase5-mesh-normal-final.log`). The focused gate-registry test suite passes all 37 tests.
- This closes only the exercised ordinary-mesh face and one boundary-edge normal/response route. Heightfield normals, other edge/corner branches, broader transformed/multi-face and mutation coverage, and full `001927`/`001929` reconstruction remain open. Public Physics headers were not changed.

### Execution checkpoint — 2026-10-03, sphere-mesh vertex branch

- Added a public sphere/mesh corner fixture with the sphere centered beyond the `(2,0,2)` mesh vertex. Oracle and candidate both report one callback and one patch at step 30, contact point `(2,0,2)`, normal words `bf073a69.bf2a2f2b.bf073a69`, separation `bcdaf02f`, and matching post-step Y position/velocity. This exercises a vertex-distance branch in addition to the previously pinned face and edge routes.
- The focused mesh differential passes with `stdout_delta=0`, exact stderr. Fresh Phase 7 passes at 1,306/1,306 (`build/phase7-mesh-vertex.log`).
- Remaining mesh work includes the other edge/corner branches, heightfield-specific normals, broader transformed/multi-face and mutation cases, and semantic closure of `001927`/`001929`. Full M1 and full-DLL completion remain active.

### Execution checkpoint — 2026-10-03, sphere/heightfield below-surface contact

- Added public-path simulation coverage for an upward-facing Y-axis heightfield with a `-100` vertical extent and a sphere initially below its surface. The ordinary mesh remains backface-rejecting; the heightfield emits the expected one-point contact. The default heightfield fixture matches the shipped DLL exactly, including contact point, signed-zero normal component, separation, post-step position, and velocity (`stdout_delta=0`, exact stderr; `build/heightfield-default-pass.log`).
- The candidate now selects one projected heightfield triangle when the sphere center projects inside the candidate set, permits the configured below-surface contact, uses the face normal for this default heightfield case, and preserves identity-transform normal bits.
- Smooth-sphere heightfield contacts and one sloped-normal case are now covered by the 2026-10-04 checkpoint below. Other vertical axes, heightfield edges/corners and extent boundaries, broader transformed/multi-face cases, and full semantic closure of `001927`/`001929` remain open. Full M1 and full-DLL completion remain active.

### Execution checkpoint — 2026-10-04, Viewer consumer tests

- Rebuilt the Release `NxPhysics` DLL and Viewer, then ran the full Viewer CTest group: all 12 tests passed, including `ViewerPhysicsStep`, `ViewerPhysicsContact`, scene loading, and the four smoke scenes. The two dedicated physics tests advance a falling box and a contacting sphere through the Viewer loop. This adds consumer-level evidence alongside the standalone oracle differential; it does not replace that gate.
- A fresh `NxPhysicsSimulationTests` oracle pair remains red only at `broadphase2-separated2`: oracle velocity `35da9eba`, candidate `35e06397` (`stdout_delta=2`, exact stderr). The Viewer tests pass but do not exercise that precise coherent-broadphase sequence, so the residual remains open.

### Execution checkpoint — 2026-10-04, smooth-sphere heightfield normals

- Added a public-path fixture using a two-triangle sloped Y-axis heightfield with `NX_MESH_SMOOTH_SPHERE_COLLISIONS`. This makes the smooth option observable: the oracle lazily builds vertex normals, interpolates them at the closest point, normalizes the world normal, and reports the sphere-surface contact point. The internal shape already stores the descriptor flag at `+0xe4`; no public header or object layout change was needed.
- `NxContactSphereMesh` now follows that path, including the oracle's x87 precision for normal interpolation/normalization and the extended-precision distance-to-separation operation. The smooth fixture's callback count, contact point, normal, separation and event words match exactly. A second fixture applies a quarter-turn and translation and also matches the world-space contact stream exactly. The existing default below-surface heightfield and ordinary mesh fixtures remain unchanged. The focused differential has `stdout_delta=0`, exact stderr; the fresh Phase 7 gate passes at 1,309/1,309 (`build/phase7-smooth-heightfield.log`).
- An exploratory output also found a tiny post-solver velocity mismatch (`oracle=0x35800000`, candidate `0`) while final Y position matched. The focused fixture pins the exact generated contact stream, not that residual solver velocity; solver response for this smooth case remains open. Other heightfield axes, edges/corners, extent boundaries, transformed/multi-face cases, mutation coverage, and full semantic closure of `001927`/`001929` remain open. Full M1 and full-DLL completion remain active. Public Physics headers were not changed.

### Execution checkpoint — 2026-10-04, initialized D6 kind-2 swing row

- Added a deterministic public D6 fixture: a sphere starts with a 60-degree rotation, one swing axis is limited to 0.2 radians, and the scene takes four 20 ms fixed steps. The descriptor explicitly sets `projectionDistance`, `projectionAngle`, and `projectionMode`; `NxD6JointDesc::setToDefault()` leaves those fields untouched. Oracle and candidate report the same final quaternion and angular velocity (`stdout_delta=0`, exact stderr).
- The older temporary D6 swing probe recorded six tiny angular-velocity word differences, but did not preserve its descriptor setup. The old residual is not reproduced by this fully initialized fixture; leave it inconclusive and retain it as a separate follow-up rather than attributing a solver bug without a valid reproducer. This case adds one registered Phase 7 assertion; focused coverage-floor tests pass and the fresh Phase 7 gate passes at 1,310/1,310 (`build/phase7-d6-swing-registered.log`).
- This covers one kind-2 swing-limit route only. Other kind-2 axis/limit interactions, the broader joint solver matrix, and repeated lifecycle interactions remain open. Full M1 and full-DLL completion remain active. Public Physics headers were not changed.

### Execution checkpoint — 2026-10-04, smooth-heightfield support response

- Added oracle-pinned post-solver position and velocity vectors for both the sloped smooth-heightfield case and its transformed counterpart. The contact stream had already matched, but these state vectors exposed residual normal-impulse ordering differences.
- Oracle disassembly and a live x87 trace identified the first-body projection/inverse-mass order in `phys_fn_004403`. `supportSolveNormal004403` now preserves that path while keeping the separate second-body sign/order branch. The ordinary mesh and below-surface cases remain bit-identical; both smooth-heightfield states now match exactly.
- The focused mesh differential and all ten Phase 7 targets pass at 1,312/1,312; the full Phase 5 runner passes its 13 staged-pair targets and both oracle-side layout/vtable differentials at 2,037/2,037; Phase 6 now also runs the mesh simulation differential and passes seven staged-pair targets plus its oracle differentials at 867/867. The CMake Release build, 80-file public-header check, focused coverage-floor tests, and inventory validation pass. Commit `d7b45a51` records the solver reconstruction and inventory proof.
- Phase 5's runner is green, while its closure ledger still defers all 205 owned functions and 122 data objects because row-level mutation evidence is outstanding. Phase 6 closes `phys_fn_004403` after body-zero Y/Z and X operation-order mutations are caught by the registered mesh target (`stdout_delta=4` each); its ledger remains at 3 closed / 430 deferred. Heightfield edge/corner and multi-face cases, the remaining M1 solver matrix, object-family audits, and full-DLL closure remain open. Public Physics headers were not changed.

### Execution checkpoint — 2026-10-04, support-normal X mutation sensitivity

- Changed the ordinary smooth-heightfield sphere's initial velocity to the inward normal direction `(0.014, -0.046, 0.014)`. This drives the body-zero X impulse close to cancellation and pins the oracle state `position=3edf07a3.3f6916e5.3edf07a4 velocity=b87e8b63.b8180000.b87f0000`.
- A throwaway archive of HEAD `7b942a40` with the updated fixture and expected line overlaid caught the X-only mutation that float-rounds the projected impulse before inverse-mass scaling. The registered `NxPhysicsMeshSimulationTests` differential reports `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=4`, `stderr_exact=True`; both ordinary and transformed state lines move. Restoring the source and rebuilding returns `stdout_delta=0`.
- The fresh Phase 6 and Phase 7 gates pass at 867/867 and 1,312/1,312 in `build/xorder-probe/phase6.log` and `build/xorder-probe/phase7.log`. Public headers are unchanged. Remaining solver, object, mesh, and full-DLL work is still open.

Current unique code-row states by owning phase:

| Phase | Discovered | Reconstructed | Dynamically gated / statically reviewed |
|---|---:|---:|---:|
| 2: SDK | 15 | 72 | 50 / 6 |
| 3: collision | 144 | 187 | 61 / 0 |
| 4: meshes/spatial/vendor | 273 | 752 | 28 / 0 |
| 5: objects | 8 | 197 | 0 / 0 |
| 6: joints/effectors | 22 | 408 | 3 / 0 |
| 7: scenes/simulation | 268 | 289 | 4 / 0 |

The eight discovered Phase 5 IDs are `000002`, `000030`, `000032`, `000034`, `002318`, `002324`, `002330`, and `002421`. Some already have candidate bodies, including `000030` and `000032`; audit their full paths and evidence before rewriting them. Phase 5 closure also requires verification of its reconstructed rows and concrete dispatch, not merely promoting these eight.

### Execution checkpoint — 2026-10-04, scene pair-count forwarding

- Routed `NpScene::getNbPairs()` through the read lock to `NxSceneInternal::getNbPairs()`. The new public simulation assertion first failed with oracle `pairs=1` and candidate `pairs=0`; after rebuilding both the DLL and test executable, the focused differential matches exactly (`stdout_delta=0`, `stderr_exact=True`). The fixture also observes `getPairFlagArray()` returning false on both sides for its contact-report record, so it does not establish pair-array behavior.
- Fresh Phase 5 and Phase 6 gates passed immediately before this edit at 2,037/2,037 and 867/867. Fresh Phase 7 after the edit passes all ten targets at 1,312/1,312; each target has zero stdout delta, exact stderr, and successful oracle/candidate exits. Logs are under `D:\FlamingEnt__\novodex-analysis\pairs\codex-main-reconstruction-1636-*`.
- The internal pair-array reader (`phys_fn_000525/000527`) and public `NpScene::getPairFlagArray()` still need an oracle-driven fixture that actually emits pair entries. Object-family/vtable closure, discovered code/data ownership, Unreal integration, and M6 full-DLL acceptance remain open. No public Physics headers changed.

### Execution checkpoint — 2026-10-04, scene pair-flag array

- Added an oracle-driven simulation case that sets `NX_IGNORE_PAIR` on a real actor pair. The oracle returns one actor pair with flags `00000001`; the reconstruction initially returned no pair array. `NxSceneInternal::getPairFlagArray()` now indexes scene shapes from the static and selected dynamic pruner pools, resolves shape IDs from the pair hash, and emits public shape or actor handles with the actor-pair bit. `NpScene::getPairFlagArray()` forwards under the scene read lock.
- Contact-report records share the scene pair hash with explicit flag records. The candidate's allocator can place contact-record pointers in the oracle's `0x20000000` marker range, so the implementation also validates the stored record's flag marker; the contact-only case stays absent while an explicit actor-flag pair is returned.
- The focused `NxPhysicsSimulationTests` differential passes with `stdout_delta=0` and exact stderr for the contact-only record, inline `NX_IGNORE_PAIR`, and record-backed `NX_NOTIFY_ON_TOUCH` cases. The fresh Phase 7 gate passes all ten targets with zero stdout deltas and exact stderr (`D:\FlamingEnt__\novodex-analysis\pairs\pair-array-phase7-final2.log`). Public Physics headers were not changed. Full Phase 5/6 closure, object-family and inventory audits, Unreal integration, and M6 full-DLL acceptance remain open.

### Execution checkpoint — 2026-10-04, shape-pair flag access

- Implemented `NpScene::setShapePairFlags()` and `getShapePairFlags()` over the existing scene pair hash. The public `NxShape` is a handle whose internal shape record is at `+8`; the oracle's internal pair routines read the shape IDs at `+0xd4` from those records. Unwrapping at the `NpScene` boundary avoids inserting a second, unrelated pair key.
- The simulation differential now covers inline `NX_IGNORE_PAIR` (getter returns `1`, pair array classifies it as a shape pair), record-backed `NX_NOTIFY_ON_TOUCH` (getter returns `8`), and clearing with zero (getter returns `0`). A pre-implementation run was red by four stdout assertions; the corrected implementation matches the oracle (`stdout_delta=0`, `stderr_exact=True`) in `shape-pair-verified-20261004.log`. The Phase 7 coverage registry includes the three new oracle outputs and its pinned floor is 1,315.
- The registry unit suite passes all 37 tests. The fresh Phase 7 gate passes all ten targets at 1,315/1,315; every differential has zero stdout delta, exact stderr, and successful oracle/candidate exits (`D:\FlamingEnt__\novodex-analysis\pairs\shape-pair-phase7-verified-20261004.log`). Same-shape error reporting, multi-shape actor-pair expansion, Phase 5/6 closure, object-family and inventory audits, candidate Unreal integration, and M6 full-DLL acceptance remain open. No public Physics headers changed.

### Execution checkpoint — 2026-10-04, compound and invalid shape pairs

- Added `NxPhysicsPairFlagTests` for a two-child actor and registered it in Phase 7. The regression compares public actor-handle identity, actor-pair classification, ordering, and flags. `getPairFlagArray()` now resolves each internal body to its public actor through the Scene actor range. The focused differential is exact (`stdout_delta=0`, `stderr_exact=True`).
- The same target calls `setShapePairFlags(shape, shape, NX_IGNORE_PAIR)`. Before the fix, the oracle reported `NXE_INVALID_PARAMETER` and preserved the pair table, while the candidate crashed. `phys_fn_000590` establishes that `Scene::setShapePairFlags` rejects identical references before the pair-hash call. Added that guard at the reconstructed Scene boundary; the candidate now matches the oracle's code, source line, message, unchanged flags/count, and absence of a self-pair.
- A focused gate-registry test caught Phase 5 coverage at 2,042 while the floor was still 2,037. Corrected the floor to 2,042; a fresh Phase 5 gate then passed at 2,042/2,042. Phase 7's registered pair-flag outputs now total 1,318; the pair-specific differential and focused registry/floor tests pass. The complete Phase 7 gate was not rerun for this checkpoint.
- Same-shape errors and multi-shape actor-pair expansion are now covered. Actor-group pair flags, the broader contact-report state machine, Phase 6 closure, object-family and inventory audits, candidate Unreal integration, and M6 full-DLL acceptance remain open. Public Physics headers were not changed.

## 2. Approach selection

1. **Recommended: dependency-driven parallel reconstruction with continuous integration.** Trace the public simulation entries to their real internal callees, partition those dependencies into owned work units, and bring up an end-to-end simulation slice while independent full-DLL work proceeds. This minimizes time to useful testing and exposes integration defects early.
2. **Sequential phase closure.** Finish every Phase 5 proof, then every Phase 6 item, then Phase 7. Easier coordination, but delays testing behind work that does not block the simulation loop.
3. **Source transcription first, testing afterward.** Maximizes the apparent rate of reconstructed rows but postpones ABI, ownership, dispatcher, and solver failures. Existing component successes alongside empty simulation entries show why this is a poor fit for the requested outcome.

Use the existing translation-unit contracts and listing bundles. Ghidra or IDA recovers structure and candidate types; Capstone listings resolve calling convention, control flow, x87 order, and ambiguous decompiles. Use cdb for execution and ownership traces. Use angr only for a specific unresolved branch or reachability problem where it saves time. Do not rerun whole-image decompilation routinely.

## 3. Milestones and dependency order

### M0 — Establish an executable backlog and integration baseline

Deliverables: corrected work-unit map, a current-main baseline report, and one dependency backlog keyed by unique stable IDs.

- Regenerate the map with the existing tool; current ownership already covers every executable row exactly once, and the committed-map test rejects duplicate names or row assignments. Keep the rollup data synchronized with inventory state changes.
- Classify all 749 discovered rows into missing, partial, implemented-but-unverified, proven vendor correspondence, or oracle-native unsupported/no-op. Locate existing candidate implementations and their callers before assigning new code.
- For reconstructed rows, record missing dispatch, unresolved callees, current-source proof, or known differential divergence separately. Avoid equating a reconstructed count with production readiness.
- Trace the public scene stepping entries into scheduling, broadphase, pair generation, contacts, islands, solver, integration, sleeping, and report delivery. Resolve indirect targets or explicitly list them as blockers. Numeric phase labels and adjacent source addresses are insufficient dependency evidence.
- Assign required data tables and globals to the same owners as their users, including vtables, dispatch matrices, allocator state, and FP control state.
- Run a clean CMake build, header check, inventory validator, tooling suite, and gates 2–7 once on a single main SHA. Record candidate hashes and exact failures. Require the Phase 5 gate to pass and retain every Phase 7 differential in the registered target set; the friction mismatch on the planning baseline has now been corrected and verified green.

Exit: every remaining ID and unresolved dependency has one owner and a test route; no duplicate work assignments; the first simulation dependency packet is ready. Timebox the initial scheduling pass to one work session and deepen contracts as each packet starts.

### M1 — Bring up the real standalone simulation path (critical path)

Primary code: `Physics/src/NpScene.cpp`, `Physics/src/Scene.cpp`, their private headers, and the actual solver/scheduler units identified by M0. Proposed test target: `NxPhysicsSimulationTests`.

- Reconstruct scene gravity, timing, writable/running state, lock and result semantics, and error paths from the oracle. Follow the oracle's relationship between old run APIs and newer simulate/fetch APIs rather than imposing a new engine design.
- Gravity and timing reads/writes have been implemented and pinned at bit level; keep this verified slice while completing the remaining scene methods.
- The initial current-main baseline had false result flags and a stationary candidate; the real scene step/result path has since been wired and now settles and collides through contact/support rows. The explicit-friction divergence is resolved. Keep filling the standalone test matrix before declaring T1 complete.
- `NxSceneDesc::broadPhase` maps the public selectors to pruning-engine modes 1/2/3. Mode 2 uses full sweep-and-prune; mode 3 uses the persistent coherent `SweepAndPrune` cache. Differential fixtures pin selector mapping, pair membership, bounded/unbounded registration, dense and chain ordering, and mode-3 move-out/move-back updates. Bounded-pruner tree-backed queries, broader update ordering, and pair/report lifecycle are still required before claiming selector behavior complete.
- Wire the recovered body state, forces, and joint implementations through the real stepping path. Complete missing solver and integration callees as a dependency cluster; do not create a substitute Euler integrator just to pass a falling-box test.
- Build independent oracle and candidate processes from identical serialized fixtures. Compare per-step poses, velocities, forces, wake state, result status, callbacks, and allocation/lifetime events.
- Start with an empty scene and one body under gravity/force, then static contact, two-body collision, kinematic interaction, sleep/wake, a small stack, and a jointed pair. Include variable step sizes and the oracle's FP control-word transitions.
- Cover each `NxSceneDesc::broadPhase` selector with bounded and unbounded scene descriptors, then prove the selected traversal emits the oracle's candidate pairs and contact outcomes.
- Exercise at least 1,000 steps for basic stable fixtures, repeated scene/SDK lifecycle, and both blocking and nonblocking result calls. Expand cases to reach every new branch before claiming its closure.
- After the standalone simulation slice passes, run the CMake Viewer as an additional consumer before Unreal integration: build `Viewer` and run `ctest --test-dir build -C Release -R '^ViewerPhysics(Step|Contact)$' --output-on-failure`. These checks advance a falling box and a contacting sphere through the viewer loop against the built SDK DLLs; record their result separately from the oracle differential gates.

Exit / **T1: standalone testing begins**: the candidate runs the genuine public step/result path through collision and solver code; the above fixtures execute without stubs, crashes, hangs, or unexplained mismatches. Any longer numerical divergence has a measured first divergent step and remains an open defect. This is not whole-DLL completion.

### M2 — Close object, mesh, and pruning ownership in parallel

Primary code: `NpActor.cpp`, `Scene.cpp`, `ObjectModel.cpp`, `TriangleMesh.cpp`, shape and pruning units. Coordinate edits to `Scene.cpp` through its owner.

- Audit the eight Phase 5 discovered rows and the real factory-to-wrapper-to-final-vtable paths for every shape family, static/dynamic bodies, meshes, compounds, controllers, and release/rollback.
- Phase 5 now passes its recorded gate. Continue the broader object audit: compare `NpActorVtable` and shape tables against the pinned census (`phys_data_000678` and `phys_data_000679`), inventory all 87/88 entries, group inherited/common slots, and add oracle-backed execution where the gate does not yet drive a concrete path. `NxPhysicsShapeVtableTests` covers shape cases but does not replace the complete actor/object-family contract.
- Replace remaining parameterized stand-ins or incomplete constructors on product paths with recovered implementations. Verify all vtable slots against the pinned oracle, including deleting destructors, base adjustments, and return ABI.
- Complete mesh loading/serialization and actual asset ownership where current tests assemble internal fixtures directly. Round-trip real asset data through the public API, not only synthetic internal objects.
- Drive dynamic shape mutation, mass recomputation, callbacks, shared ownership, allocation failure, and populated-scene teardown through the rebuilt DLL. Verify allocator choice and ordering.
- Regenerate the vtable census and attach current execution evidence. Replace the unconditional Phase 5 marker with executable dispatch/ownership assertions only when the whole family it represents is covered. A passing count or a deleted marker alone does not close Phase 5.

Exit: Phase 5 passes honestly, required object/mesh/pruner paths have no unknown dispatch, and M1's lifecycle and contact scenarios pass through these real objects. M1 and M2 exchange fixtures early; M1 does not wait for unrelated closure paperwork.

### M3 — Complete collision, spatial, vendor, and joint integration

- Finish the remaining collision/pruning/penetration-map clusters and joint/articulation/solver dependencies selected by the call graph. Prioritize anything M1 reaches or blocks on.
- Audit already vendored OPCODE/qhull correspondence before rewriting those rows. Preserve pinned upstream trees and use documented overlays for oracle-specific changes.
- Turn important direct-function comparisons into public-path scenarios: primitive/mesh contacts, mesh/mesh, compound shapes, filters/materials, continuous collision where present, raycast/overlap/culling, joint limits/motors/breakage, and effectors during simulation.
- Review existing divergence ceilings. Record actual divergent inputs and causes; frozen nonzero ceilings are useful regression controls but are not proof of exact reconstruction. Resolve them or prove the difference is outside the oracle contract before final acceptance.

Exit: every recovered shape pair and joint family participates correctly in simulation; filters, contact streams, callbacks, and spatial queries agree with the oracle for the recorded corpus.

### M4 — Complete the remaining full-DLL surface

Run independently of the rigid-body critical path once shared lifecycle contracts are stable.

- Reconstruct fluids, fluid emitters/managers, implicit meshes, controllers, notification/reporting, remaining scene configuration/statistics, visualization, and serialization/export paths present in this exact binary.
- Distinguish genuine oracle no-op/unsupported slots from reconstruction placeholders using the binary and behavior. Reproduce unsupported behavior when it is the actual contract; do not invent functionality based only on public header declarations.
- Test fluids/controllers through their public scene lifecycle, interactions, callbacks, and destruction. Include SDK/scene release while those objects exist.
- Audit remaining SDK exports, initialization/teardown, error and assertion behavior, allocator crossing, and late/uncommon paths.

Exit: every in-scope public and internal behavior has an implementation or evidence-backed oracle-native classification. No subsystem is excluded because Unreal rarely invokes it.

### M5 — Unreal integration and regression feedback

Begins after T1 and the actual consumer's required public paths are covered; it may overlap M3/M4. Proposed runner: `run_consumer_smoke.ps1`.

- Inventory imports and observed calls of the Unreal executable and its Novodex consumers under `D:\FlamingEnt__\Unreal_3`; identify a reproducible launch/map workflow and compatible mesh assets.
- Stage a recoverable test installation of the rebuilt Physics/Foundation pair, with recorded original/candidate hashes and automatic restoration. Keep the original oracle installation available for comparisons.
- Start with SDK initialization, map load, scene creation, one simulated scene, and shutdown. Extend to map transitions, rigid-body interactions, joints, mesh collision, queries, and fluids/controllers where the consumer uses them.
- Capture crashes, first state divergence, callback order, allocation problems, and performance regressions as owned reconstruction defects. Consumer coverage adds to the standalone corpus; it does not replace full-DLL verification.

Exit / **T2: Unreal testing begins**: reproducible load/simulate/unload runs succeed with the staged candidate pair, without fallback to the original Physics DLL. Testing then continues throughout remaining work.

### M6 — Full reconstruction acceptance

Proposed artifacts: `run_final_verification.ps1`, registered Phase 8 targets, final source/data ownership report, reproducible build manifest, and handoff notes.

- Clean configure and build from the recorded source revision; verify all 41 exports, ordinals, calling conventions, public header hashes, concrete vtables, imports, Foundation linkage, and candidate-owned data relocations.
- Audit all 6,338 function records, distinguishing executable code from compiler/runtime/other classified records. Audit all 5,138 data records by ownership, alias, generated equivalent, or legitimate non-runtime classification. No unowned required code/data, unresolved indirect target, unimplemented product path, or silently deferred required row remains.
- Complete the original plan's closure/falsification requirements. A trace hit proves execution; a matching fixture proves that fixture; neither alone proves all semantics. Exercise mutation sensitivity at meaningful boundaries and use whole-body listing/relocation checks where appropriate.
- Run all phases 2–8, the complete tooling suite, standalone simulation corpus, resource stress/soak cases, and Unreal regression suite on the final pair. Final verification rejects absent/skipped required gates and unresolved divergence.
- Confirm the release package runs with the oracle unavailable, contains the CMake-built DLL pair, and includes hashes, build instructions, test commands, and remaining limitations (none that contradict full reconstruction).

Exit: the full objective is demonstrated on the final source/build. Green component gates or an Unreal launch alone do not satisfy this milestone.

## 4. Work allocation for speed

Suggested capacity assumption: four concurrent implementers, with one also owning integration. This is a proposed execution arrangement, not a dispatch performed by this planning task.

| Owner | Initial responsibility | Boundary |
|---|---|---|
| A: simulation/integration | M0 baseline and M1 public step-to-solver chain | Owns NpScene and scene orchestration; defines shared lifecycle contracts |
| B: objects/assets | M2 object finals, meshes, ownership, Phase 5 closure | Publishes narrow interfaces to A; coordinates Scene.cpp edits |
| C: collision/spatial | M3 missing collision/pruner/vendor clusters | Owns corresponding source units and oracle fixtures |
| D: remaining subsystems/verification | M4 fluids/controllers; harness and M5 preparation when dependencies block | Does not rewrite A's scheduler or shared ownership abstractions |

One integrator owns shared `inventory.json`, gate registries, closure ledgers, and generated work-unit output. Implementers deliver row-level evidence/registration patches, which the integrator reconciles against current main. This directly addresses the duplicated map and differing historical gate floors seen in the baseline.

Use one isolated worktree per owner, stable-ID ownership, small dependency-complete commits, and frequent integration. Do not assign the same shared file independently without a named merge owner. If capacity is lower, retain this order and reduce concurrency rather than weakening acceptance.

## 5. Execution rules that reduce wasted work

- **One packet per coherent call-graph/source cluster**, ordinarily about 5–12 KB of oracle code; split large anonymous gaps after caller/callee analysis. Avoid another sequence of unrelated one-method probes.
- **Audit before transcription.** Reuse existing bodies, listings, bundles, and fixtures. Promote evidence only after checking current source, not from old branch status.
- **Wire immediately.** A reconstructed function must be reached by the correct product caller, or its intentional standalone role must be documented. Do not postpone integration until a directory is complete.
- **Test at the right cadence.** Run the changed target and its direct dependency targets per edit; run the affected phase per packet; run gates 2–7 and tooling at integration checkpoints; clean builds at milestone boundaries. Repeat broader suites only after new changes or unresolved failures justify them.
- **Freeze fixtures and expectations from the oracle.** Register input coverage as well as results; prevent tests from passing through constant answers, empty paths, or skipped callbacks.
- **Avoid broad refactors.** Introduce private units only where required to own a recovered subsystem or isolate shared-file contention. Preserve public ABI and the existing CMake topology.
- **Measure time to verified behavior.** Track time in analysis, implementation, differential debugging, and integration separately. Count verified product-path rows and passing end-to-end scenarios; source bytes alone are not a completion metric.

Largest remaining discovered clusters for initial triage (regenerated map; names indicate address neighborhoods, not proven subsystem boundaries):

| Cluster | Rows | Bytes |
|---|---:|---:|
| IcePrunable → OPC_MeshInterface gap | 58 | 40,204 |
| Controller → Fluid gap | 91 | 29,058 |
| ContactPlaneMesh → PenetrationMap gap | 43 | 20,213 |
| TriangleMesh → Controller gap | 24 | 16,739 |
| Fluid → FluidManager gap | 46 | 14,051 |
| SphereShape → ConvexHull gap | 22 | 10,004 |
| Scene.cpp | 42 | 6,849 |
| TriangleMesh.cpp | 25 | 6,647 |
| Shape.cpp | 18 | 6,528 |

These queues do not establish the solver's location. M0 must resolve that from executable dependencies rather than treating a neighboring source-file name as ownership.

## 6. Scheduling and review checkpoints

Do not promise a completion date from the 749-row count. It mixes existing implementations awaiting proof, vendor correspondence, small thunks, and complex missing algorithms. Previous timing tables include approximated starts and integration overlap, so extrapolating a single rows/hour rate would be misleading.

The first execution checkpoint is M0 plus one complete scene-stepping dependency packet. At that point report: remaining critical-path dependency clusters, observed packet throughput, verification cost, and an optimistic/likely/pessimistic forecast for T1 and full completion. Reforecast after T1 using actual collision/solver behavior. Keep the first testing date separate from the full-DLL completion date.

Review order:

1. Approve this project scope, dependency order, and proposed parallel ownership.
2. Produce the short execution contracts for M0/M1 and each independent workstream, with concrete stable IDs, files, test entry points, and dependency handoffs. Reuse existing contracts wherever current main already satisfies them.
3. Select execution capacity and begin; integrate at each dependency-complete packet.
4. Review T1 standalone results, then T2 Unreal results, then the full M6 acceptance report.

The original full-DLL goal stays intact throughout these intermediate milestones.

The user has selected full-DLL reconstruction and the test order (standalone simulation, then Unreal). Do not hold the work on another scope decision. For fastest useful feedback, keep the passing Phase 5 and Phase 7 gates green, continue M1's public-path solver/lifecycle matrix to its T1 exit, and start the Unreal smoke as soon as that standalone exit is met. M2–M4 continue in parallel by call-graph ownership; full acceptance remains M6.

### Follow-up execution checkpoint — 2026-10-01

- Closed the second low-force fixed-joint break callback outcome. A new public-API simulation fixture installs an `NxUserNotify`, checks the callback receives the same public joint and oracle force bits, and verifies a `true` return automatically releases that joint during `fetchResults`. The candidate previously returned null from `getUserNotify`, never dispatched the callback, and retained the broken joint. `NpScene::setUserNotify` and `getUserNotify` now use the scene's descriptor-confirmed `+0x6ac` pointer with the wrapper locks. The focused differential is exact (`stdout_delta=0`, `stderr_exact=True`; `build/notify-break-green.log`).
- A second fixture covers a `false` return: the oracle invokes the callback once, reports the same joint and force, leaves the joint broken and enumerable, and allows the application to release it explicitly. Candidate output is exact (`build/notify-break-false-differential.log`). The eight callback outcome lines are registered in Phase 7, whose floor is now 1,236. A fresh Phase 7 run passes all eight targets and registered coverage (`build/phase7-notify-break-final.log`); Phase 5 passed after the scene API change (`build/phase5-notify-break.log`).
- The two focused gate-registry modules pass all 44 tests (`Ran 44 tests in 201.451s OK`). A fresh full tooling run then passed all 770 tests in 217.9 seconds, and inventory validation still reports 6,338 functions, 5,138 data objects, and zero unexplained entries. After refreshing D6 expectations, `test_gate_targets` also passes all 37 tests (`208.268s`).
- Added a public `NxPointInPlaneJointDesc` gravity fixture to cover the remaining kind-2 `004397` dispatch through the real scene scheduler. All 12 actor states match the oracle bit-for-bit (`stdout_delta=0`, `stderr_exact=True`; `build/point-in-plane-first-diff.log`). The observations are registered, and a fresh Phase 7 run passes at 1,249/1,249 assertions (`build/phase7-point-in-plane.log`). This establishes the exercised kind-2 route, not coverage of every joint configuration.
- Added a public D6 fixture with all three linear axes locked against an identity-oriented world frame. The 12-step output matches exactly (`build/d6-joint-identity-locked-diff.log`), and the refreshed Phase 7 gate passes at 1,262/1,262 assertions (`build/phase7-d6-identity-locked.log`). An all-axis rotated-frame setup diverged after the first step, so rotated D6 motion combinations and kind-5 dynamic coverage remain open; this closes only the tested identity-frame route.
- Added three repeated public SDK/scene lifecycle cycles. Each cycle creates an SDK, one scene, and one dynamic actor; advances and fetches a simulation step; releases the scene; checks that the scene count is zero; and releases the SDK. The full simulation target matches the oracle exactly (`stdout_delta=0`, `stderr_exact=True`), including all three cycle records. The refreshed Phase 7 gate passes all eight targets at 1,265/1,265 (`build/phase7-d6-identity-locked.log` refreshed on this worktree); this closes the tested empty-SDK release cycle, while release with live joints/effectors/controllers and repeated scene creation within one SDK remain open.
- The agreed testing sequence is standalone simulation coverage first, then the Unreal integration run. Continue M1 by expanding scenario/solver-kind coverage and the remaining lifecycle combinations; use the standalone differential suite as the T1 exit evidence before scheduling T2 in Unreal.
- The 45-degree D6 world-frame fixture (three locked linear axes, 12 fixed steps) is now green. After setting projection fields explicitly to zero / `NX_JPM_NONE`, the remaining mismatch was traced to inverse-pose `row004180`: the oracle preserves two intermediate values in x87 80-bit precision, while MSVC's C++ path spills them as qwords. The x86/MSVC implementation now preserves those intermediates; all 12 rotated-D6 observations match and are registered in Phase 7 (floor 1,278).
- The all-axis angular-lock D6 mismatch was traced past row generation to `004395`'s angular-only body update. The oracle retains the first two inverse-inertia row sums in x87 registers until after adding current angular velocity, while the candidate rounded all three deltas to float first. `supportApplyAngularImpulse004395` now reproduces that x86/MSVC precision boundary. All 12 orientation/angular-velocity states and the completion record match the oracle exactly (`stdout_delta=0`, `stderr_exact=True`; `build/d6-angular-x87-retain2.log`) and are pinned in Phase 7.
- Fresh Phase 7 passes all eight targets at 1,291/1,291 coverage assertions (`build/phase7-angular-final.log`). Fresh Phase 5 passes all 13 targets at 2,037/2,037, including the repaired Phase 5 red gate (`build/phase5-angular-final.log`). The D6 angular fixture is now registered; other D6 configurations and the broader object/vtable audit remain open. No public Physics headers changed.
- Added the missing populated-scene lifecycle case: two dynamic actors joined by a live fixed joint complete one fixed step, then the scene is released without first destroying the actors or joint. Oracle and candidate both report `actors=2 joints=1 scenes=1.0`, exit cleanly, and match exactly (`stdout_delta=0`, `stderr_exact=True`; `build/populated-lifecycle-first-diff.log`). The observation is registered in Phase 7; all eight targets pass at 1,292/1,292 (`build/phase7-populated-final.log`). This covers a live-joint teardown path; effectors/controllers, multiple scenes in one SDK, and allocation-failure rollback remain open.
- Added two populated scenes under one SDK, each with two dynamic actors and a live fixed joint; both scenes step and are released in reverse creation order. The SDK reports scene counts `2 → 1 → 0`, and the complete transcript matches the oracle exactly (`stdout_delta=0`, `stderr_exact=True`; `build/multi-scene-lifecycle-diff.log`). Phase 7 registers this observation and passes at 1,293/1,293 (`build/phase7-lifecycle-expanded-final.log`). This closes the tested multi-scene ordering case; populated teardown with controllers and allocation-failure rollback remain open.
- A fresh Phase 5 run after the lifecycle fixtures still passes all 13 targets at 2,037/2,037 (`build/phase5-lifecycle-expanded-final.log`); `git diff --check` is clean. These new cases expand Phase 7 evidence without changing the current Phase 5 acceptance result.
- No public `Physics/include` headers were changed. The DLL still requires the remaining M1/M2/M3/M4 work and the broader T2 Unreal regression matrix before full reconstruction can be claimed.
- The M1 standalone smoke threshold has now been reached, so T2 was started with matched, isolated Unreal test roots under ignored `build/unreal-consumer-smoke`; the installed oracle and candidate binaries were never overwritten. The selected `DemoGame.exe PhysTest` map resolves and loads in both roots. The oracle run reaches `Game engine initialized` with zero `URB_BodyInstance::InitBody` or `Scene.cpp:552` actor-init failures, then encounters a fatal UI/exit condition that is not yet diagnosed; therefore this is not a clean oracle baseline or a passing T2 run. On the same map, the candidate reports 24 body-init failures and six actor-init failures before its fatal UI/exit condition. This establishes a real candidate divergence during level construction, while the fatal exit itself remains an environment/baseline investigation item.
- **T2 smoke correction — 2026-10-03:** The earlier staged roots contained the September 20 `DemoGame.exe`, not the October 1 executable now installed under `D:\FlamingEnt__\Unreal_3\Binaries`; both stale roots failed with `Bad expr token 46`, so that comparison is superseded. After refreshing the isolated roots with the current executable and current binary dependencies, the oracle and CMake candidate both loaded `PhysTest`, initialized the game, unloaded the map, and exited 0. Process module snapshots confirm each process loaded NxPhysics and NxFoundation from its own staged root. Both runs emitted the same four non-kinematic center-of-mass diagnostics, also present on the oracle. The evidence and hashes are in `docs/reconstruction/novodex-physics/evidence/demogame-phystest-smoke-2026-10-03.md`. This starts T2 with one reproducible map smoke; the wider M5 matrix and all M1/M2/M3/M4/M6 work remain open.
- **Qhull QRn parity — 2026-10-03:** A failing exact regression first captured the QRn rotation mismatch (32 discrete and 1,890 x87 differences). Overlaying the reference's one-reciprocal Gram-Schmidt normalization in `External/qhull/novodex/geom2.c` made the QRn discrete/x87 tapes and 9,431-word rotated hull tape exact; all three ceilings were removed and the gate now requires exact verdicts. The pinned upstream source is unchanged and the overlay/license manifest passes. Phase 4 passes 251/251; fresh Phases 5/6/7 pass 2,037/2,037, 856/856, and 1,293/1,293. Rebuilt candidate SHA-256 is `d4bc7fd9ce0aec1d9f72c54eb8b55a1fa0b225810e9835edd2c230920d2cb2922`; a refreshed staged DemoGame `PhysTest` run exits 0 and loads both candidate modules from its own directory. The four-word set-16 `qhull_paths` distance-test counter mismatch and other Qhull ceilings remain open; the full M1/M2/M3/M4/M6 scope is unchanged.
- The first confirmed product blocker is the public convex-mesh path: `NpPhysicsSDK::createTriangleMesh` currently returns null (`Physics/src/NpPhysicsSDK.cpp`), and the SDK-side factory, `TriangleMesh` constructor and `TriangleMesh::loadFromDesc` are also absent. The existing contract (`docs/reconstruction/novodex-physics/units/convex-cooking-contract.md`) already maps the dependency chain: `000242 → 000478 → 002251 → 002260`, with built hull support `002233` and follow-on mesh build/model paths. Prioritize this coherent cooking/ownership packet next, add a public-API convex-cooking differential that goes red first, then rerun the staged PhysTest pair. Do not attribute every one of the 24 failures to this path until the first failing asset is traced.
- Added `NxPhysicsConvexMeshTests` and registered the first Phase 4 staged-pair assertion. The test creates a valid tetrahedral convex mesh and queries its cooked arrays. The pinned DLL reports `created=1 submeshes=4 vertices=4 triangles=4 hull_vertices=4 hull_polygons=4`; the current build reports `created=0`, returns failure, and the paired runner records `oracle_exit=0 candidate_exit=1 stdout_delta=2` (`build/convex-api-red.log`). This is the watched RED for the absent factory/object chain, not a product fix. The test's two oracle-side identity hashes match the pinned installed pair; the candidate identity matches the worktree Release DLL. Phase 4's coverage floor is incremented for this new assertion.
- Implemented the first InternalTriangleMesh allocation/lifetime packet in `Physics/src/InternalTriangleMesh.cpp`: rows 002065, 002067, 002069, 002071, 002073 and 002075 now have source bodies. Static proof covers measured initialization boundaries, zero-triangle no-op allocations, vertex/triangle/material/remap byte counts, release order/count, cleared pointers, and preserved counts. Corrected the previously incomplete +0x1c layout description: phys_fn_002079 allocates a 16-byte-per-triangle record there, and 002067 owns its release. `NxPhysicsInternalTests` passes 74 checks. Its target now includes the simulation and pruning source dependencies revealed by a fresh relink (`Island.cpp`, `BodyStep.cpp`, `ContactPairManager.cpp`, `NovodexBoxPruning.cpp`); the fresh Phase 2 gate passes build, static proof, and all three differentials. Inventory validation passes at 6,338 functions / 5,138 data objects / zero unexplained. The public convex test remains red on the rebuilt DLL with `created=0`; next close 002079/002083 and wire the factory/constructor/load lifecycle before retrying Unreal. No public Physics headers changed.

- Follow-up: implemented 002079's plane build using the vendored `IceMaths::Plane::Set` at the recovered call edge, and 002083's embedded `MeshInterface` setup, OPCODE `Model` allocation/build, and release lifecycle. The pinned image's `phys_data_003482` bytes at 0x00123b3c are zero, matching the `OPCODECREATE::mKeepOriginal` default. `NxPhysicsInternalTests` now passes 77 checks, including a valid single-triangle model build. Inventory validates with 6,338 functions / 5,138 data objects / zero unexplained, and the public header manifest still passes all 80 files. This is a static proof of the one-triangle path; the public convex factory differential is still RED until the missing `TriangleMesh`/`NpTriangleMesh` construction and descriptor-loading chain is wired.
- Verification after adding 002079/002083: Release `NxPhysics` and `NxPhysicsInternalTests` build, the internal proof passes 77/77, all 80 public headers pass the manifest, inventory validates, and a fresh official Phase 5 run passes all 13 staged targets at 2,037/2,037. The official Phase 4 convex cooking pair still shows the known RED (`oracle created=1`, candidate `created=0`, exit 0/1, stdout delta 2), confirming the next critical path is the absent public mesh factory/construction/descriptor loader; no Unreal retry until that probe turns green.
- Current continuation (2026-10-01): the public tetrahedron path is now implemented through `NxPhysicsSDK::createTriangleMesh`, the SDK factory, `TriangleMesh`/`NpTriangleMesh` construction, descriptor cooking, internal arrays/model, convex hull, wrapper queries, and release. The standalone API probe now reports `created=1 submeshes=4 vertices=4 triangles=4 hull_vertices=4 hull_polygons=4`; `NxPhysicsInternalTests` passes 77 checks, inventory remains 6,338 functions / 5,138 data objects / zero unexplained, and immutable public headers remain 80/80. This proves only the tetrahedron path, not all mesh flags, descriptors, PMaps, or ownership branches. The Phase 4 build and asset differential build cleanly. Its full gate still fails in `NxPhysicsThirdPartyTests`: the prunable-pruner row now compares exactly after fixing the fixture's stack-backed box array teardown, but the harness later corrupts its `selfOnly` argument during `nxDrivePrunableDispatch` and crashes at convex cooking. This is a test-harness issue still to isolate; do not weaken or skip the registered gate. Next, isolate/fix that harness memory corruption, rerun Phase 4, then broaden the public mesh corpus (strides, 16-bit indices, flip normals, materials, rejected descriptors, and repeated create/release) before retrying Unreal.

- 2026-10-01 follow-up: Phase 4's apparent convex-cooking crash came from the test harness calling the oracle's `nxDrivePrunableDispatch` with the wrong calling convention. Disassembly evidence showed `ret 8`; switching to the measured register/stack setup stops the per-call ESP drift. The full harness now reaches cooking. Phase 4 remains red on real qhull/create/compute and direct-qhull output divergences, which need source reconstruction and measured differential evidence rather than relaxed ceilings.
- The public mesh corpus now includes computed and precomputed tetrahedra, padded strides, 16-bit indices/materials, flip normals, repeat create/release, array accessors, and `saveToDesc`. Oracle comparisons establish that convex hull faces keep original descriptor winding while `NX_MF_FLIPNORMALS` changes the exposed triangle winding; hull normals come from `ConvexHull::mVertexNormals`, and hull polygon format is `NX_FORMAT_INT`. Candidate now matches the flipped case exactly and no longer crashes. Computed/precomputed array order still differs, so the new Phase 4 target is not green yet. Keep Phase 5's established 2,037/2,037 pass intact and reverify before integration claims.
- The four `_obj` text families now match the pinned CRT output after signed-zero normalization at the OBJ writer boundary; `build/qhull-zero-sign-final.log` records exact text and byte tapes. This removes the known formatting-only difference without changing geometry. The remaining red Phase 4 evidence is still substantive: box/cluster hull paths, direct qhull box output, and direct x87 geometry comparisons remain open. The candidate-mismatch aggregate decreased from 8 to 4, but Phase 4 is not green.
- Diagnostic refinement: the optional `NXHULL_PROBE` mode now prints each differing x87 tape word. On the current clean rebuild (`build/qhull-x87-words-probe.log`), set 12 (box) differs in 105 discrete qhull words plus 40 x87 words. Set 17 (clusters) has exact discrete output through direct qhull but 21 differing low words in the double tape; the public hull result remains structurally divergent. The mismatch is therefore split between box topology and cluster numeric geometry. Fresh CMake build succeeded; Phase 4 remains red at four candidate mismatches. Public headers remain unchanged.
- Fresh-process stability and address attribution: three new oracle runs have identical `hull_qhull_direct` / `hull_create_qhull` digests. The `NXHULL_PROBE` trace in `build/qhull-address-probe.log` shows the box-case oracle and candidate reuse the same raw vertex object addresses but assign those slots to points in different orders (oracle IDs 5–8 → points 5,3,6,7; candidate → 3,5,7,6). The pointer-hash hypothesis may amplify a prior ordering divergence; it is not yet proven as the first cause. Next, trace the first differing insertion/merge decision rather than changing the hash or setting a tolerance. The fresh non-probe gate remains red at four candidate mismatches (`build/qhull-final-current.log`); OBJ text output is exact. Public headers pass 80/80; `git diff --check` is clean.
- 2026-10-01 continuation: pinned disassembly at `qh_distplane` RVA `0x0005c5c0` revealed the first box divergence: oracle and candidate planes match, but the candidate tied two furthest distances that differ by one low-order bit in the oracle. `geom.c` now matches the measured 3D arithmetic grouping, and rebuilt candidate instructions match the oracle. The direct box qhull tape's discrete words are now exact (105 → 0 mismatches); its x87 tape still has 22 mismatches, and public `CreateConvexHull` / `computeHull` qhull families remain divergent. The wrapper tapes additionally show two same-sized temporary blocks released in opposite order before the output delta. Fresh full Phase 4 third-party run remains red with 13 candidate mismatches and zero layout failures; no ceilings were broadened. A fresh Phase 5 run passes all 13 targets at 2,037/2,037 (`build/phase5-resume-check.log`). This preserves the repaired Phase 5 gate while Phase 4 continues as the active blocker.
- Correction after the point-level public probe: the public oracle/candidate calls do not reach qhull with the same cleaned points for box set 12. Their eight two-point-weld fallback corners differ by low bits in x/z (`build/qhull-resume-points-probe.log`); the earlier shared digest came from the direct-qhull test's single candidate-cleaned buffer fed to both sides. Thus the first public-path blocker is now localized to `QhullHost::cleanupVertices`'s welded two-point fallback. A trial volatile-spill change had no effect and was reverted; the point-dump diagnostic is opt-in. The temporary-block free-order difference is downstream. Continue with oracle-vs-candidate fallback arithmetic reconstruction.

- 2026-10-05 viewer coverage and slot-growth checkpoint: Viewer CTest now discovers all 39 checked-in `.pds.ods` demos, copies the scene asset tree into the build directory before running, and serializes smoke runs against that staged copy so generated dumps do not alter source assets. The expanded 44-test selection (scene and sound asset setup, 39 scenes, sound smoke, and two focused physics tests) passes 100%: 34 scene demos, sound, both focused physics tests, and both setup tests pass; four PMap scenes and `TruckDemo` are marked skipped only when their verified pinned-oracle failure text appears. `ConvexTest16` initially exposed candidate-only memory corruption after shape ID 255. Oracle disassembly at `0x5bc90` shows 256-entry slot-vector growth; the candidate now grows its per-slot tables at that boundary and the scene's active-slot vector when full. `ConvexTest16` now passes. Fresh Phase 5 passes at 2,042/2,042 assertions (`build/phase5-main-viewer-slots.log`). The standalone `NxPhysicsSimulationTests` differential remains red with `stdout_delta=2`; the remaining known row is `broadphase2-separated2` velocity (`oracle 35da9eba`, candidate `35e06397`), while stderr remains exact (`build/simulation-after-viewer-slot-growth.log`). Phase 4 remains red on qhull/convex reconstruction; no Unreal retry or completion claim is warranted yet.
- The user approved the all-scenes Viewer test design. Rebuilt current-source verification on 2026-10-05 again completes all 44 selected CTest cases: 39 pass and the five pinned-oracle failures are reported as skipped (`build/phase5-scene-design-approved.log` records the accompanying Phase 5 run). Phase 5 passes with 2,042/2,042 coverage assertions. The simulation differential remains red on the single `broadphase2-separated2` velocity row; an attempted contact angular-precision change increased other diffs and was reverted, leaving the established source behavior intact. Phase 4 qhull/convex work remains open.
- 2026-10-05 Phase 4 closure: the CDB cleanup trace localized the asset harness crash to its borrowed hull sentinel. `nxCandidateMeshWriter` used a stack buffer address to make the hull block present, then `TriangleMesh::~TriangleMesh` interpreted it as owned `TriangleMeshConvexData`; the helper now detaches that sentinel after `save()`. Oracle/candidate asset differential passes with zero mismatches. Refreshed the third-party expected coverage totals to the current exact oracle transcript; all 215 Phase 4 oracle-side coverage assertions remain registered and the aggregate floor stays 259. Fresh `run_phase_gate.ps1 -Phase 4` passes (`build/phase4-final-current.log`): asset candidate mismatches 0, third-party candidate mismatches 0/layout failures 0, convex-mesh staged differential exact, and the triangle-mesh API/settle staged differential exact. The standalone simulation differential still has its one `broadphase2-separated2` velocity mismatch (`35da9eba` oracle, `35e06397` candidate); public headers remain unchanged.

- 2026-10-05 approved viewer scene design rerun: current Release CTest enumerated and ran every one of the 39 checked-in .pds.ods scenes after staging the assets. 34 scene tests passed; CowPile, PMapTest8/10/12, and TruckDemo were skipped by their verified pinned-oracle failure patterns. The staging fixture passed, so all 40 selected CTest entries completed with 100% success and no unexpected scene failure. Fresh Phase 5 also passes 2,042/2,042 (build/phase5-after-phase4-closure.log). This verifies viewer startup/load smoke coverage; it does not replace the standalone simulation differential, which remains red on broadphase2-separated2, or close the remaining full-DLL reconstruction work.

- 2026-10-05 primitive trigger dispatch repair: the Phase 7 trigger differential was red because the candidate overlap dispatch table omitted the already-reconstructed primitive overlap kernels. Wiring sphere/sphere, sphere/box, sphere/capsule, box/box, box/capsule, and capsule/capsule entries makes the box-trigger/moving-sphere fixture deliver the oracle's exact 20 ENTER/STAY/LEAVE callbacks (stdout_delta=0, stderr_exact=True; build/trigger-fixed-pairs and build/phase7-trigger-fixed.log). Dispatch-only Phase 5 harnesses now provide inert link stubs for those kernels, matching their existing mesh-kernel stub arrangement; production simulation targets still link the real overlap code. Fresh Phase 7 passes all registered 1,263 assertions and Phase 5 passes all 2,042 (build/phase5-trigger-fixed-rerun.log). This closes the tested primitive trigger-pair route, not all trigger geometry, callback lifecycle, or full-DLL reconstruction.
- 2026-10-05 scene-design approval regression check after primitive trigger wiring: ctest --test-dir build -C Release -R Viewer --output-on-failure completes 48/48 registered Viewer tests successfully (100%; 43 passed plus the same five pinned-oracle skips) in 240.51 seconds (build/viewer-scenes-trigger-fixed-release.log). This includes all 39 available scene demos and focused Viewer physics-step/contact checks. The initial no-configuration CTest invocation is superseded; the Release-configured run is the valid result.
- 2026-10-05 compound actor-pair teardown: the new release regression first exposed a candidate access violation in `getPairFlagArray()` after `releaseActor()` left a pair keyed by the freed compound group ID. Tracing showed runtime shapes bypass `ShapeBase::nxBaseDtorOwnerArms`; `nxRuntimeShapeBaseDestroy` now calls `NxSceneRemoveOwnerPairRecords` before recycling that ID. The empty-pair return also matches the oracle. The focused differential is exact (`stdout_delta=0`, `stderr_exact=True`; `build/pairflag-owner-cleanup-final.log`), and its post-release result is registered in Phase 7. Fresh Phase 7 passes 1,264/1,264 (`build/phase7-owner-cleanup-final.log`), Phase 5 passes 2,042/2,042 (`build/phase5-owner-cleanup-final.log`), and the Release Viewer selection passes 48/48 with the five existing pinned-oracle scene skips (`build/viewer-scenes-owner-pair-cleanup.log`). No public Physics headers changed. This closes the exercised compound-actor pair-map teardown route; other owner-map lifecycle paths remain subject to the broader M2/M3 audit.
- 2026-10-05 approved all-scene Viewer design follow-up: the Release Viewer selection still completes all 48 registered cases, covering all 39 checked-in scenes plus setup and focused physics tests; 43 pass and the same five pinned-oracle failures skip by their verified failure signatures (`build/viewer-scenes-owner-pair-cleanup.log`). The standalone simulation oracle differential has since been rerun on the current candidate and now matches exactly (`stdout_delta=0`, `stderr_exact=True`; `build/simulation-resume-current.log`), superseding the older broadphase2-separated2 mismatch above. Fresh Phase 6 passes all registered checks with 867/867 coverage assertions (`build/phase6-scene-approved-baseline.log`). These results advance the viewer and simulation test baselines; M1's remaining lifecycle and scenario matrix, and the full-DLL audit, remain open. Public Physics headers remain unchanged.
- 2026-10-05 NxScene overlap-query slice: the oracle-backed regression exposed that `checkOverlapSphere` and `checkOverlapAABB` returned false unconditionally. Their public read-lock wrappers and internal selected-pruner traversal now match the oracle on static/dynamic/all selectors, hits, misses, the unbounded plane, rotated-box corner rejection, primitive sphere/box/capsule cases, and tangent hit/miss boundaries. The focused `NxPhysicsSceneRaycastTests` differential is exact (`stdout_delta=0`, `stderr_exact=True`; `build/scene-overlap-tangencies.log`); Phase 7 passes 1,276/1,264 coverage assertions (`build/phase7-scene-overlap-final.log`), Phase 6 passes 867/867, and Phase 5 passes 2,042/2,042. The rebuilt Viewer selection passes 48/48, with the same five pinned-oracle scene skips (`build/viewer-scenes-spatial-overlap.log`). Inventory validates at 6,338 functions / 5,138 data objects / zero unexplained. `checkOverlapSphere` triangle-mesh behavior and `overlapSphereShapes`, `overlapAABBShapes`, `cullShapes`, and `overlapAABBTriangles` remain open; the full DLL is not complete and public Physics headers remain unchanged. See `docs/reconstruction/novodex-physics/evidence/scene-spatial-overlap.md`.
- 2026-10-05 approved scene-query design continuation: reconstructed `NpScene::overlapAABBShapes` with the internal selected-pruner collector, world-AABB refresh, unbounded plane handling, bounded output, and callback-only batches. The differential for full array, capacity-one truncation, and callback output matches exactly (`build/overlap-callback-contract.log`). Phase 7 passes at 1,280 coverage assertions (`build/phase7-overlap-collection.log`). `overlapSphereShapes`, `cullShapes`, `overlapAABBTriangles`, and mesh behavior in `checkOverlapSphere` remain open; this advances the scene-query work without implying full-DLL completion. No public Physics headers changed.
- 2026-10-05 scene-query follow-up: reconstructed `NpScene::overlapSphereShapes` using the recovered sphere broadphase/narrow-phase route and the same buffer/callback reporting contract. The focused differential returns the expected box and unbounded plane through both array and callback (`stdout_delta=0`, `stderr_exact=True`; `build/sphere-overlap-callback.log`). Fresh Phase 7 passes all targets at 1,282 coverage assertions (`build/phase7-scene-collections.log`), and the Release Viewer sweep passes 48/48 with all available scenes exercised (43 passed plus five known pinned-oracle skips; `build/viewer-scenes-scene-collections.log`). Public Physics headers remain unchanged. Mesh overlap, `cullShapes`, `overlapAABBTriangles`, and the broader scene-query pruning slots remain open; the DLL reconstruction is still in progress.
- 2026-10-05 culling follow-up: reconstructed `NpScene::cullShapes` and its selected-pruner plane collector. The new oracle case confirms the retained half-space is non-positive, bounded box selection, unbounded plane preservation, and callback delivery; the staged-pair differential is exact (`build/cull-callback.log`). The ten scene-query wrapper/internal census rows now have reconstruction and dynamic-differential evidence and are correctly recorded as reconstructed-not-falsified rather than discovered; work-unit rollups were regenerated and match their tests. Fresh Phase 7 passes at 1,285/1,264 assertions (`build/phase7-cull-final.log`). The mesh-overlap and AABB-triangle query gaps remain open; all public Physics headers remain untouched.
- 2026-10-05 AABB-triangle query continuation: reconstructed the public wrapper and an internal actor/mesh traversal that appends world-space triangles, including translated actors; a SAT check rejects an AABB inside a triangle's bounds but outside the triangle. Oracle cases cover a prefilled array, ordinary and corner misses, all triangles from two registered meshes, and translated output. The focused differential is exact (`stdout_delta=0`, `stderr_exact=True`; `build/overlap-tri-diff.log`). Five markers are required by Phase 7 coverage on both pairs; the full gate passes at 1,290/1,264 assertions (`build/phase7-overlap-tri.log`). The Release Viewer sweep after the change passes 48/48 registered tests across all available scenes and focused Viewer physics tests (43 pass plus five verified pinned-oracle skips; `build/viewer-scenes-overlap-tri.log`). Broader triangle-order and edge behavior, row-specific mutation falsifications, mesh handling in `checkOverlapSphere`, the pruning-engine slots, and the full DLL audit remain open. No public Physics headers changed.
- 2026-10-05 approved all-scene Viewer design and mesh sphere-overlap continuation: all 39 checked-in Viewer scenes remain in the Release smoke selection, with five verified pinned-oracle failures skipped by their signatures; after this code change the complete Viewer selection is 48/48 successful (43 passed, five skipped; `build/viewer-scenes-sphere-mesh-overlap.log`). `checkOverlapSphere` now handles mesh shapes via the world bounds; oracle cases hit a sphere inside those bounds but away from the mesh triangles, reject a sphere outside the bounds, and verify the static/dynamic/all selectors. The focused differential is exact (`stdout_delta=0`, `stderr_exact=True`; `build/sphere-mesh-overlap.log`), and Phase 7 passes at 1,294/1,264 coverage assertions (`build/phase7-sphere-mesh-overlap.log`). Inventory validation passes at 6,338 functions / 5,138 data objects / zero unexplained; all 80 public headers match the pinned manifest. Rotated mesh bounds, near-boundary cases, row-specific mutation falsification, remaining pruning behavior, and the full DLL audit are still open. The approved scene sweep is testing evidence, not full reconstruction completion.
- 2026-10-05 transformed mesh-bound query evidence: `NxPhysicsSceneRaycastTests` now covers a 90-degree rotated and translated mesh, a bounds-only hit away from triangle geometry, a zero-radius tangent hit, and a near-boundary miss. Oracle and candidate transcripts match exactly (`stdout_delta=0`, `stderr_exact=True`; `build/rotated-mesh-query-differential.log`). The three new Phase 7 markers are registered; the full gate passes at 1,297 evaluated assertions against the corrected 1,297 floor (`build/phase7-rotated-mesh-overlap-final.log`). The inventory records this expanded dynamic proof for `phys_fn_000672`. This closes only those transformed-bound query cases; row-specific mutation falsification, remaining pruning-engine semantics, and full-DLL reconstruction remain open. No public Physics headers changed.
- 2026-10-05 static AABB tree-query follow-up: `StaticPruner` slot 8 (`phys_fn_005229`) now runs OPCODE's `AABBCollider` against its cached tree and reports candidates in its tree traversal order. The scene array collector uses this path for static shapes while retaining the recovered dynamic-pruner walk. A twelve-box fixture with permuted insertion order plus the unbounded plane matches the pinned oracle exactly (`stdout_delta=0`, `stderr_exact=True`; `build/bounded-query-differential.log`), producing `tree_11` through `tree_00` followed by `s_plane`. Inventory row `phys_fn_005229` is now reconstructed with this differential evidence. The Release Viewer sweep then passed all 48 registered tests, exercising all 39 available scenes; the same five pinned-oracle failure cases were signature-verified skips (`build/viewer-scenes-bounded-pruner.log`). Bounded-dynamic tree query slots 5–8, cull and sphere-query pruning traversal, row-specific falsification, and the full-DLL audit remain open. Public Physics headers remain unchanged.
- 2026-10-05 Viewer scene-run isolation follow-up: the combined scene-load and physics-step/contact tests now use the same disposable staged scene tree as the 39 per-demo smoke tests, and share its resource lock to prevent concurrent dump writes. Release CTest passes all 48 Viewer selections (43 passed, five signature-verified pinned-oracle skips) in 80.21 seconds (`build/viewer-scenes-staged-all-final.log`); `ViewerScenes/PhysicsStepSmoke/dump1.psc` remained clean after the run. The bounded-pruner query's strict all-hit order remains an open differential: oracle order begins `bounded_05, bounded_10`, while the candidate begins `bounded_00, bounded_05` (`build/raycast-order-oracle.log`, `build/raycast-order-candidate.log`). A simpler tree-versus-fallback classification was tested and rejected because it classified all twelve shapes as fallback; that speculative code and its diagnostics were removed. This adds repeatable, source-clean Viewer startup and step smoke coverage; it does not close the broader reconstruction or the separate full audit.
- 2026-10-05 bounded-tree order closure: strict all-hit printing is now part of `NxPhysicsSceneRaycastTests`. Raw oracle node evidence confirmed a root cube twice the stored bounds, center-based child-cell bounds, depth selection from the stored tree scale, and two output streams (partially intersected cells before fully contained subtrees). `BoundedDynamicPruner::OverlapAABB` now models those rules; the staged differential is exact (`stdout_delta=0`, `stderr_exact=True`; `build/raycast-pruner-strict-green.log`). Phase 7 passes all 11 targets at 1,298/1,298 registered assertions, including a new exact-order coverage marker (`build/phase7-bounded-strict-registered.log`). The full Release CTest suite passes all 56 selections: all 39 Viewer scene cases were exercised, 51 tests passed, and five exact-signature oracle asset cases were skipped; no failures (`build/ctest-all-scenes-final.log`, 185.01 seconds). Updated three stale late-ray coverage literals to match the oracle's `d_box` tie result. `git diff --check` passes; no public Physics headers changed. This closes the strict fixture only, not every bounded-pruner configuration or the full DLL audit.
- 2026-10-05 approved scene-query continuation: restored the scene sphere-query dispatch through pruner slot 8 while retaining AABB slot 7, implemented static sphere traversal with OPCODE's cache, and added bounded-octree sphere traversal with the oracle's sphere/AABB pruning, strict per-object predicate, child order, and contained-subtree stream. `overlapSphereShapes` now returns broadphase candidates in pruner order; the boolean `checkOverlapSphere` keeps its separate narrow shape test. The new moved-actor sphere-order probe and all existing scene-query records match the staged oracle transcript exactly (`stdout_delta=0`, `stderr_exact=True`; `build/sphere-pruner-complete-oracle.log`, `build/sphere-pruner-complete-candidate.log`). Fresh Phase 5 passes at 2,042/2,042 assertions; fresh Phase 7 passes at 1,298/1,298. The approved Viewer sweep covers all 39 checked-in demo scenes: 34 pass and five known pinned-oracle asset failures are signature-skipped; Viewer scene-load/step/contact tests also pass 4/4 (`build/sphere-pruner-complete-*.log`, `build/phase5-scene-worktree.log`, `build/phase7-sphere-viewer-worktree.log`). The aggregate repository CTest command also enumerates unrelated graphics/sound tests whose executables were not built; those were `Not Run`, not failures, and the Viewer-scoped runs above are the relevant results. Remaining pruner configurations, row-specific mutation falsification, and full-DLL acceptance stay open. No public Physics headers changed.
- 2026-10-05 scene statistics and limits: implemented `getSceneStats` and `getLimits` in the NpScene/Scene path without changing public headers. `NxPhysicsSimulationTests` now checks empty and populated scenes for all three broadphase selectors; the populated case confirms 8 actors, 5 bodies, 3 static shapes, 5 dynamic shapes, and 0 joints. Its staged oracle differential is exact, and a fresh Phase 7 gate passes all 11 targets at 1,310/1,310 assertions with zero stdout deltas. The test registry pins all 12 observed lines and its focused floor check passes. Evidence: `evidence/scene-stats-limits.md`, `build/SceneGateFinal-gate.log`. The full DLL, including row-specific mutation closure, remains unfinished.
- 2026-10-05 disabled fluid contact-report API: reconstructed `phys_fn_000362` and `phys_fn_000364` as the pinned release's unavailable setter/getter. The setter ignores the callback and both methods emit the exact `NXE_DB_WARNING` tuples; the getter returns null. The test was red first (candidate omitted both warnings), then the focused staged simulation differential passed exactly (`stdout_delta=0`, `stderr_exact=True`; `build/FluidGate/fluid-report-green.log`). Phase 7 passes all targets at 1,313/1,313 (`build/FluidGate/phase7-fluid-report.log`), the inventory validates at 6,338 functions / 5,138 data objects / zero unexplained records, and the gate-registry suite passes 37/37. Public headers remain unchanged. Full fluid and implicit-mesh lifecycle paths remain open.
- 2026-10-05 disabled implicit-mesh scene API: reconstructed the four public create/release/count/list methods as the pinned build's unavailable contract. Each warning tuple and null/zero result matches the oracle; initial RED omitted all four warnings, then the staged differential passed exactly (`stdout_delta=0`, `stderr_exact=True`; `build/FluidGate/implicit-mesh-green.log`). Phase 7 passes 1,317/1,317 (`build/FluidGate/phase7-implicit-mesh.log`), both public-header hash checks pass, and inventory validates with 6,338 functions / 5,138 data objects / zero unexplained records. Enabled implicit-mesh behavior and the remaining fluid manager APIs are still open.
- 2026-10-05 disabled fluid release lifecycle: reconstructed `NpScene::releaseFluid`, Scene's reentry-guarded manager release/clear path, and the disabled manager warning. The RED test showed no warning and a retained manager; the oracle showed `NXE_DB_WARNING` 206 at FluidManager.cpp line 183 and cleared Scene +0x61c. With the empty-manager non-member probe, the focused differential is exact (`stdout_delta=0`, `stderr_exact=True`; `build/FluidGate/fluid-release-green.log`). Phase 7 passes 1,318/1,318 (`build/FluidGate/phase7-fluid-release.log`) and Phase 5 passes 2,042/2,042 (`build/FluidGate/phase5-fluid-release.log`). Inventory validation passes at 6,338 functions / 5,138 data objects / zero unexplained. Live-fluid list removal, destructor dispatch, and reentry remain open because the pinned build cannot create a fluid. No public headers changed; see `evidence/fluid-release-disabled.md`.

- 2026-10-05 disabled FluidManager construction/destructor follow-up: the constructor now writes the oracle-observed empty array headers, Scene link, backend flags, and initialized flag without zeroing conditional storage. A callable first vtable slot implements scalar deleting teardown and is exercised by a direct internal vtable test. The focused simulation oracle/candidate transcripts match across 1,025 normalized lines; candidate and oracle both exit 0. Evidence: vidence/fluid-release-disabled.md, build/FluidGate/fluid-vtable-candidate-green.log, and build/FluidGate/fluid-vtable-oracle-green.log. Live-fluid/nonempty-array behavior and extension-backed setup remain open. The approved full Viewer sweep also freshly passed all 48 Viewer selections: 43 passed and five existing signature-verified scene skips, including every registered demo scene and both physics checks. The full DLL goal remains active.

- 2026-10-05 Phase 7 follow-up after the disabled FluidManager test: the isolated full gate rebuilt NxPhysics and all 11 Phase 7 targets, matched every oracle/candidate transcript, and passed with 1,320/1,320 coverage assertions (build/FluidGate/phase7-fluid-manager-vtable-gate.log). The coverage floor is now pinned at 1,320 in both the gate registry and its unit tests. Public headers remain unchanged; live-fluid and extension-backed behavior remain open.

- 2026-10-06 approved Viewer scene sweep against the rebuilt DLL: built Viewer and all registered support executables in the isolated Phase 7 build, then ran the complete Viewer CTest selection. All 48 selections passed; 34 of 39 scene demos ran, five existing exact-signature oracle cases were skipped, and both Viewer physics checks passed (build/FluidGate/viewer-scenes-after-fluid-manager.log, 49.45 seconds). This validates Viewer scene startup/load and smoke behavior plus the focused step/contact integrations; longer per-scene runtime remains outside this smoke gate.
- 2026-10-06 FluidManager populated-array release follow-up: a seeded two-entry fixture first showed the candidate removed the requested fluid from both arrays but omitted the oracle's scalar-deleting vtable call. `nxFluidManagerReleaseDisabledFluid` now dispatches on the exact removed object after swap-removal; the exact oracle differential passes (`stdout_delta=0`, `stderr_exact=True`; `build/FluidGate/fluid-array-target-final-differential.log`). Inventory row `phys_fn_003643` now records this verified slice as reconstructed, its work-unit rollups were regenerated, and the work-unit consistency tests pass 12/12. The full Phase 7 gate passes all 11 targets at 1,321/1,321 assertions (`build/FluidGate/phase7-fluid-array-target-final.log`), the coverage-floor unit tests pass 2/2, and Phase 5 passes all 13 targets at 2,042/2,042. The Viewer sweep passes all 48 registered selections against this build, including all 39 scenes and the five existing signature-verified skips (`build/FluidGate/viewer-scenes-after-fluid-array-release.log`). This covers manager array mutation and deleting dispatch with a test-seeded fake object; a real `NpFluid`, its destructor body, extension-backed initialization, and emitter ownership remain open. Public Physics headers are unchanged; full-DLL reconstruction remains active.
- 2026-10-06 disabled FluidManager actor notifications: creating and releasing a public sphere actor after a failed `createFluid` call first produced a focused RED: the oracle reported the line-263 create-actor and line-250 release-actor feature warnings, while candidate was silent. Scene now forwards those notifications to the disabled-manager warning helpers; the exact simulation differential passes (`stdout_delta=0`, `stderr_exact=True`; `build/FluidGate/fluid-actor-notify-green.log`). Inventory rows `phys_fn_003635` and `phys_fn_003637` remain discovered because the full array iteration/callback behavior is not reconstructed; the disabled empty-array warnings are recorded as partial dynamic evidence. Their live-fluid callback paths through `phys_fn_003485` remain open. The Phase 7 gate passes 11 targets at 1,323/1,323 (`build/FluidGate/phase7-fluid-actor-notify.log`), and Phase 5 passes 13 targets at 2,042/2,042 (`build/FluidGate/phase5-fluid-actor-notify.log`). The approved complete Viewer selection passes 48/48 (`build/FluidGate/viewer-scenes-after-fluid-actor-notify.log`): 34 scene demos ran and five existing exact-signature asset cases skipped; Viewer physics step/contact also passed. Public headers remain unchanged and full-DLL reconstruction is still active.
- 2026-10-06 `NpScene::setShapePairFlags` now follows the recovered row's write-lock/call/unlock protocol before forwarding unwrapped public shape handles to `Scene`. The focused `NxPhysicsPairFlagTests` differential remains exact (`stdout_delta=0`, `stderr_exact=True`); a fresh Phase 7 run passes all 1,330 registered coverage assertions, and the Release Viewer selection passes 48/48 (43 passed, five existing signature-verified pinned-oracle skips) across all 39 demos and both focused physics checks. This verifies ordinary pair-flag behavior and the rebuilt Viewer; the lock-contention branch still lacks a dedicated differential fixture. The full-DLL reconstruction remains active.


Continuation 2026-10-06: reconstructed the write-lock failure diagnostics for `NpScene::setActorPairFlags` (`phys_fn_000309`, line `0xab`) and `setShapePairFlags` (`phys_fn_000313`, line `0xb8`). A second thread holds the scene write lock while the public calls run; both were RED against the generic candidate warning and are now exact, including unchanged pair flags. One-line source-location mutations on each row are caught (`stdout_delta=2` each). Inventory/work-unit rollups and Phase 7 closure are updated; validation reports 6,338 functions, 5,138 data objects, zero unexplained; Phase 5 passes 2,042/2,042 (`build/scene-pair-lock-phase5.log`), and Phase 7 passes 1,332/1,332 (`build/scene-pair-lock-final-phase7.log`). Focused registry/map checks pass 4/4. The Release Viewer suite passes 48/48 after rebuilding the matching DLL pair: all 39 scenes are included, 43 tests pass, and the same five pinned-oracle asset failures skip (`build/scene-pair-lock-viewer-ctest.log`). Public Physics headers remain unchanged; full-DLL reconstruction remains active.

Continuation 2026-10-06: completed the same contention branch for `NpScene::createActor` (`phys_fn_000293`, line `0x69`), `releaseActor` (`phys_fn_000295`, line `0x70`), and `createJoint` (`phys_fn_000297`, line `0x78`). Their focused scene-lock fixture first went RED: the oracle reported all three `NXE_INVALID_OPERATION` diagnostics while the candidate emitted only generic warnings. The reconstructed diagnostics now match the oracle exactly, with create/release/joint state unchanged (`build/scene-create-lock-green.log`). Each one-line mutation is independently caught (`stdout_delta=2`; logs `build/scene-create-actor-lock-mutation.log`, `build/scene-release-actor-lock-mutation.log`, and `build/scene-create-joint-lock-mutation.log`). The pair-flag/scene-mutation fixture registers all three reports plus the no-mutation outcome; Phase 7 floor increased by four. Phase 7 closure and unit records now include these three rows. Public Physics headers remain unchanged; full-DLL reconstruction remains active.

Validation for the scene-lock slice: full Phase 7 passes all 11 differentials and 1,336/1,336 registered coverage assertions (`build/scene-create-lock-phase7.log`); Phase 5 passes all 15 targets and 2,042/2,042 assertions (`build/scene-create-lock-phase5.log`). The fresh Release Viewer run passes all 56 CTest selections over all 39 registered scenes and focused Viewer physics checks; 51 tests pass and the five pre-existing PMap/CowPile/TruckDemo oracle-baseline cases skip (`build/Testing/Temporary/LastTest.log`). The standalone registry and work-unit checks pass 4/4. Public Physics headers remain unchanged; full-DLL reconstruction remains active.

Continuation 2026-10-06: `NpScene::fetchResults` (`phys_fn_000398`) has partial dynamic evidence from the existing simulation callback fixture. The registered `simulation fetch-callback summary` now pins callback dispatch; suppressing `Scene::processSimulationCallbacks` changes the candidate transcript (`stdout_delta=877`). However, a mutation suppressing `cpmDeliverBufferedContactReports` leaves the transcript unchanged (`stdout_delta=0`), revealing that the buffered-report arm is not covered. Inventory and work-unit records classify the implemented wrapper as reconstructed, while the closure ledger keeps it deferred until a test drives buffered contact-report delivery.

The strengthened Phase 7 run passes all 11 staged differentials and 1,337/1,337 registered assertions (`build/scene-fetch-results-phase7.log`). The exact simulation callback transcript passes after restoring the mutation (`build/scene-fetch-results-green.log`); the buffered-report branch remains explicitly open.

Continuation 2026-10-06: probed the adjacent `NpScene::releaseFluid` forwarding path. A temporary local mutation that skipped `Scene::releaseFluid` was caught by the registered simulation differential (`stdout_delta=4`; `build/scene-release-fluid-mutation.log`), and the source was restored and rebuilt. The result is recorded as supplemental sensitivity evidence, not Phase 7 closure, because it was run in the active checkout and the pinned SDK cannot create a real `NxFluid`. A createFluid mutation exposed a fixture limitation: when its manager disappears, the test exits before reporting the staged module identity, so the differential runner properly rejects the run before measuring a delta. No createFluid mutation proof is claimed. Both rows remain open for full backend semantics; see `evidence/fluid-release-disabled.md`.

Continuation 2026-10-06: isolated the `phys_fn_000640` actor-contact callback branch by replacing its `contactReport->onContactNotify` call with a no-op in a throwaway archive of HEAD `0ac9f256`. The fresh archive build ran through the registered Phase 7 simulation pair gate: oracle and mutant both exited 0, stderr matched, and the mutant transcript differed by 937 normalized lines (`build/scene-contact-callback-branch-archive-mutation.log`). The Phase 7 closure ledger now records this row as falsified. `NpScene::fetchResults` (`phys_fn_000398`) remains open until its second delivery helper is observed independently; see `evidence/fetch-contact-callback-mutation.md`.

Continuation 2026-10-06: fixed and closed `NpScene::fetchResults` (`phys_fn_000398`). The new two-fetch fixture proves that a buffered contact report remains queued without a listener and is delivered once after listener registration; it first went RED (`pending=0`, `calls=0`, `stdout_delta=4`). Oracle decompilation showed the candidate's extra `cpmDeliverBufferedContactReports` call was not part of the fetch path and discarded reports when no listener existed, so that call was removed. Phase 7 passes all 11 differentials and 1,338/1,338 coverage assertions (`build/scene-fetch-deferred-contact-phase7.log`). A throwaway-archive mutation suppressing `processSimulationCallbacks` is caught (`candidate_exit=1`, `oracle_exit=0`, `stdout_delta=944`, exact stderr; `build/scene-fetch-results-callback-archive-mutation.log`). See `evidence/fetch-results-mutation.md`.

Continuation 2026-10-06: completed the next controller-mesh sweep slice following the approved scene design. Continuous SAT now checks the axis-aligned controller box against transformed triangles. The corner fixture first went RED (oracle x=2/flags=0, candidate x=1/flags=4); after the mesh narrow-phase, both pass through. A paired actual-face hit fixture reports exact stop position x=1, y=z=0.2 and flag 4. Simulation transcript is exact (`stdout_delta=0`, `stderr_exact=True`; `build/controller-mesh-final-diff.log`). Both mesh observations are asserted and registered; the Phase 7 floor is 1,351. The verified fixture uses float vertices and 32-bit triangle indices; implementation decoding of 16-bit indices still needs paired coverage. Other formats, transformed controllers, overlap recovery, sliding/stepping, callbacks, and complete controller behavior remain open. Public Physics headers remain unchanged; full-DLL reconstruction remains active.

Continuation 2026-10-06: extended the controller-mesh SAT slice with initial-overlap behavior. Two isolated oracle scenes place a controller centered on the triangle-mesh plane and move in opposite X directions. The oracle escapes fully to x=1.0 and x=2.0 with no flags, while the first candidate failed both (`stdout_delta=4`, both stopped at x=1.5/flag 4). The SAT helper now omits a triangle already overlapping the controller at sweep start; the focused differential is exact (`stdout_delta=0`, `stderr_exact=True`; `build/controller-mesh-overlap-isolated-diff.log`). Both outcomes are registered in Phase 7, increasing its floor to 1,353. Phase 5 passes 2,042/2,042 and Phase 7 passes 1,353/1,353. The rebuilt Viewer passes 44/44 selected CTest cases covering all 39 scenes and focused physics checks. This is evidence for the tested mesh plane only; generalized mesh penetration recovery and controller step/slide behavior remain open.

Continuation 2026-10-06: added the approved grounded controller step-probe scene. The oracle and candidate stop at the same pose `(0.5, 0.5, 0)`, but the initial candidate reports flags `0x6` where the oracle's separate resolver probes report `0x5`; the paired fixture was RED before the candidate preserved the descriptor's enabled step setting and remapped this tested flag combination. The focused differential now matches exactly (`stdout_delta=0`, `stderr_exact=True`; `build/step-scene-green-diff.log`). Phase 5 passes 2,042/2,042, Phase 7 passes 1,355/1,355, the complete Viewer selection passes all 48 entries (34 scenes pass, five existing signature-verified oracle asset cases skip, and both focused physics tests pass), the tooling suite passes 770 tests, and inventory validation reports 6,338 functions / 5,138 data objects / zero unexplained. This closes only the blocked grounded probe flag case; successful step-up motion and the broader controller behavior remain open. Evidence: `docs/reconstruction/novodex-physics/evidence/controller-grounded-step-probe.md`. Public Physics headers remain unchanged and full-DLL reconstruction remains active.

Continuation 2026-10-06: corrected the controller descriptor audit after checking the pinned IDA database. `Scene::createController` (`sub_1005A880`) checks only the type word at descriptor `+8` before allocating and constructing. `phys_fn_002324` (`0x5a240`) is `NxBoxShapeDesc::isValid`, as recorded in `evidence/box-shape-desc-validation.md`; it is not a controller descriptor method. A malformed negative-dimension raw controller descriptor crashes the pinned oracle before it emits a completion transcript; the exact fault stage was not isolated, so that probe is not safe differential evidence for either row. The positive-step-offset grounded sweep remains blocked at `(0.5, 0.5, 0)` with flags `0x5` on both binaries; successful step-up remains unresolved. The reverted probe build of `NxPhysicsSimulationTests` returned exact oracle/candidate transcripts (`stdout_delta=0`, `stderr_exact=True`; `build/controller-desc-baseline-after-revert.log`). Keep controller descriptor validity as a separate open contract; do not attribute it to `phys_fn_002324`.

Continuation 2026-10-06: added two rotated-box initial-overlap exits to the controller scene matrix. With a controller centered inside a 45-degree box, the oracle moves a full unit in either X direction with no flags; the candidate initially remained at the start with side flag 4. The OBB SAT path now omits obstacles containing the starting controller center, matching both transcripts. The focused simulation differential passes exactly; Phase 7 passes 1,361/1,361, Phase 5 passes 2,042/2,042, and the complete Release Viewer selection passes 48/48 (43 passed, five existing signature-verified oracle asset skips) across all 39 scenes. See `docs/reconstruction/novodex-physics/evidence/controller-rotated-initial-overlap.md`. This is one tested initial-overlap case, not full transformed-controller support; successful step-up, callbacks, and remaining controller behavior remain open. Public headers remain unchanged.

Continuation 2026-10-06: compared the controller constructor's descriptor copies with its move probes. The oracle stores descriptor up-axis `+0x20` as private object `+0x14` and step offset `+0x2c` as private object `+0x20`; the candidate initially left both zero. The candidate now copies these fields, and the public-scene regression asserts the values and a blocked step-offset move. The focused simulation differential is exact, Phase 5 passes 2,042/2,042, Phase 7 passes 1,363/1,363, and the complete Release Viewer selection passes 48/48 over all 39 scenes (43 pass; five existing signature-verified oracle asset skips). Evidence: `docs/reconstruction/novodex-physics/evidence/controller-descriptor-motion-state.md`. This closes constructor state only; the recorded obstacle still blocks on both binaries, so successful step-over, full transformed movement, callbacks, and broader controller semantics remain open. Public Physics headers remain unchanged.
