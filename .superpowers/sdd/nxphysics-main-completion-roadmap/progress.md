# SDD ledger — plan: docs/superpowers/plans/2026-09-30-nxphysics-main-completion-roadmap.md

Ruling: use the roadmap's Immediate Unreal-blocking packet as the active implementation task — the roadmap defines milestones and exits, not numbered task briefs; this packet is the narrowest user-prioritized route to completing standalone testing and the next concrete blocker.

Task M4-mesh-plane: the deterministic public mesh-contact fixture now matches exactly; broader mesh cooking, serialization, and lifetime work remain open. Current branch is `main`, with pre-existing modifications and untracked canaries; preserve them.

TDD RED: `cmake --build build --config Release --target NxPhysicsTriangleMeshApiTests` then run the test against both pair directories. Oracle exits 0 and rests at `y=0x3f73332a` with zero velocity; candidate exits 1 because it falls through (`y=0xc183a6d8`, `vy=0xc19ba6e0`). The failing assertion predates this task and was verified in this turn.
Evidence update: after wiring plane/mesh contact, the candidate now rests but differs by one ULP. Geometry hashes revealed the earlier source of differing contact ordering: cooked vertex and triangle buffers differ from the oracle (vertices FNV b358b4da777eaee5 vs 6a3c86cf2364d2e5; triangles de3e8705a22195a4 vs 4ea943683c23e915). Pivoting within the same M4 packet to the convex cooking/hull output dependency before deciding whether the contact kernel needs further adjustment.
Ruling: permit a one-ULP vertical settle delta in the immediate mesh-contact smoke gate — the oracle and candidate expose the same eight cube vertices and twelve triangles with identical per-triangle coordinates but different vertex numbering from the known 0x027f qhull facet-order divergence; the candidate settles with zero velocity at 0x3f73332b versus oracle 0x3f73332a. Cost if wrong: this gate could admit a small contact-order error; the qhull and full mesh differential remain explicitly open and final acceptance still requires resolving/classifying it.

Correction 2026-10-08: the preceding one-ULP ruling is superseded. Fresh staged
oracle/candidate runs match the cube arrays and settle bits exactly at
`0x3f73332a`; the API test now requires exact bits. A separate 400-point public
cook probe exposed process-dependent reduced-hull topology in the oracle, so
the public fixture checks construction, geometry bounds, usable topology, and
allocation balance while the direct Phase 4 oracle differential exercises its
separate recorded qhull inputs and algorithm behavior. Broader reducer fidelity
stays open.

Update 2026-10-01: Fixed the current Phase 5 red gate in the managed worktree. Inventory validation now passes after removing the resolved ContactPlaneMesh source allowlist entry and removing unsupported `implementation` mappings from `phys_fn_000242`/`000244` (they remain `discovered` until behavior is falsified). `NxPhysicsObjectLayoutTests` and `NxPhysicsShapeVtableTests` link again with focused test-only stubs for unexercised mesh/contact hooks. Fresh `run_phase_gate.ps1 -Phase 5` passes 2,037/2,037; both public-header checks pass (80 files each).

Update 2026-10-01: `NxPhysicsTriangleMeshApiTests` now exits 0 for the installed oracle and worktree candidate. Both create the mesh actor with matching mass/inertia and settle at zero velocity. Candidate y `0x3f73332b` differs by one ULP from oracle `0x3f73332a`; keep the explicit ruling above and qhull vertex-order fidelity open.

Update 2026-10-01: Candidate DemoGame probe now crashes during `Physics.war` map initialization with an access violation at null. Interactive CDB showed candidate code loading vtable entry `[object+0] + 0x24` then calling null. The live table is the `nxShapeGroupTable` from `Physics/src/Scene.cpp`; existing slots 0/4/6 point to its destructor, mass, and owner-update callbacks, while slot 9 is zero. Stack return RVAs were candidate `0x7b287`, `0x22ff3`, and `0xedb2`; exact caller contract still needs confirmation. Installed original NxPhysics/NxFoundation hashes were verified restored after every probe.
Correction to the preceding Unreal finding: inventory data object `phys_data_000847` pins the original group dispatch table at `0x10106c2c`, size 60 bytes/15 slots, with non-null targets only in slots 0-7; slot 9 is null in the oracle too. Therefore do not implement a new slot-9 method. The next design is to trace why the candidate calls that slot on a group and correct the caller/object classification to match oracle behavior.

Update 2026-10-01: Fixed the DemoGame null virtual call in `Physics/src/ContactGeneration.cpp`. CDB traced it to `NxContactCompoundShape` asking `NxShapeWorldBounds` for the compound shape group's aggregate bounds; the oracle group table has a null slot 9 for that query. The candidate now compares each child box with the other leaf shape's bounds and preserves the null slot. A focused regression was red first (`matrix contact_compound ... wrong=1`) and green after the fix (`wrong=0`).
Verification: `NxPhysicsCollisionTests` passes; `NxPhysicsSimulationTests` oracle/candidate differential passes with identical output; fresh Phase 5 gate passes at 2,037/2,037 and verifies both 80-file public-header manifests; DemoGame Physics.war candidate smoke exits 0 after benchmark and normal shutdown. Installed oracle Physics/Foundation hashes were restored and verified after the engine test. Candidate pair hashes at test: Physics `3f4aa48cd9aea87a89028d22822236fd643274e406497baf6142b476f62ff396`, Foundation `9972a8e9f182c1a3c3acf6e4be364c176a83d0e0b0490d3825de9da493c81a11`.
Status: the immediate Unreal-blocking packet is closed and first T2 smoke succeeds. M4 mesh cooking/serialization/lifetime and broader Unreal behavior remain open; preserve known one-ULP settle and qhull vertex-order debt.


Task M4-pmap-compute: Ruling: pin the valid resolution-32 compute path rather than density 8 — the pinned cell-run decoder/encoder sets coordinate widths only for 32, 64, and 80; density 8 produces oracle bytes but uses a zero-width absolute escape and is not a valid round-trip fixture — cost if wrong: malformed unsupported-density output remains untested as a strict byte contract.

Task M4-pmap-resolution64: complete on current `main`. Fresh isolated density-64 runs now pass on both oracle and candidate at size 74,563 and FNV `2c38820e277e9465`; the earlier stale candidate mismatch above is superseded by the current evidence in `docs/reconstruction/novodex-physics/evidence/pmap-resolution64.md`.

Task M4-pmap-resolution80: complete for the authored tetrahedron. The new `NxPhysicsPMapResolution80Tests` target pins the oracle's size 144,272 and FNV `1c6814b928a39f10`. Phase 4 differential passes with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and exact stderr. Evidence: `docs/reconstruction/novodex-physics/evidence/pmap-resolution80.md`. Other PMap topologies, resolutions, and creation arms remain unverified. Current branch is `main`; the approved all-scenes Viewer gate across all 39 available demos remains part of the verification design.

Update 2026-10-08: A fresh Release build on current main exposed literal backslash-n bytes at the end of Physics/src/NpActor.cpp, introduced by fe998028; MSVC reported C2017/C1004. Removed only that suffix and the surplus trailing blank line. NxPhysics and NxPhysicsTriangleMeshApiTests rebuild successfully. A freshly staged current pair (candidate NxPhysics 0c30d943..., oracle 4b7db3e1...) both exits 0; all 21 non-loader diagnostic lines match exactly, including identical cube cooked-array hashes and settled y 0x3f73332a with zero velocity. Phase 5 remains pending; this does not close broader qhull/mesh cooking work.

Update 2026-10-08: The revised public 400-point cloud passed 20 fresh processes
on each DLL pair with stable success/topology markers; 12-process repeats also
matched cooked mesh arrays and exact `0x3f73332a` contact settle bits. Phase 4
passed 268/266 coverage assertions; Phase 5 passed 2,563/2,563 after fixing
`run_differential.ps1` to reject matching nonzero child exits. A forced dual
failure proved the old runner false-pass and the corrected runner failure.
Broader mesh/reducer and full-DLL reconstruction remain open.

Update 2026-10-08: Capsule destructor row `phys_fn_001014` is mutation-falsified;
the fresh Phase 5 gate passes all 19 staged targets at 2,564/2,564 assertions.
The approved Viewer selection passes all 48 entries and exercises all 39
available scene demos: 43 pass and five known signature-verified oracle asset
cases skip. Logs: `build/phase5-capsule-dtor-final-pass.log` and
`build/viewer-all-scenes-approved-design-final.log`. Phase 5 still has 33
reconstructed rows without mutation proof; full DLL reconstruction remains
open.

Task M2-owner-notify-001271: direct oracle/candidate adapter probe added. The
slot-9 mutation is detected (`candidate_slot=9; mismatches=1`); restored output
calls slot 10 once and forwards the same owner and AABB. Phase 5 is now 173
closed / 32 deferred; the fresh full gate passes at 2,565/2,565 assertions.
Evidence:
`docs/reconstruction/novodex-physics/evidence/phase5-owner-notify-001271.md`.

Task M2-shape-dtors-001263-001375: targeted flag-1 self-free mutations for
the plane and sphere destructors were independently detected by the existing
oracle-backed allocator-delta cases. Both restored runs return the pinned
shape-vtable digest with zero mismatches. Phase 5 advances to 175 closed / 30
deferred; its fresh full gate passes at 2,565/2,565 assertions. Evidence:
`docs/reconstruction/novodex-physics/evidence/phase5-plane-sphere-dtors.md`.

Task M2-box-dtor-000979: suppressing the box shape's flag-1 self-free is
detected by the existing oracle-backed shape-vtable allocator check
(`mismatches=1`); the restored target returns to the pinned digest and zero
mismatches. Phase 5 advances to 176 closed / 29 deferred; the fresh full gate
passes at 2,565/2,565 assertions. Evidence:
`docs/reconstruction/novodex-physics/evidence/phase5-box-dtor-000979.md`.

Task M2-mesh-dtor-001399: suppressing the mesh shape's flag-1 self-free is
detected (`frees=2/1; mismatches=1`); the refcount decrement 7 to 6 remains
correct and the restored target matches exactly. Phase 5 advances to 177
closed / 28 deferred; the fresh full gate passes at 2,565/2,565 assertions.
Evidence:
`docs/reconstruction/novodex-physics/evidence/phase5-box-mesh-dtors.md`.

Task M2-owner-notify-001271: complete (commit `0c388b28`; mutation target
failed on slot 9 and restored target passed; Phase 5 passed 2,565/2,565).
Task M2-shape-dtors-001263-001375: complete (commit `2db00eec`; both targeted
mutations failed as expected, restored shape-vtable differential passed, and
Phase 5 passed 2,565/2,565).
Task M2-box-mesh-dtors-000979-001399: complete (commit `35ea4f56`; both
targeted mutations failed as expected, restored destructor checks passed, and
Phase 5 passed 2,565/2,565).

Task M2-mesh-ctor-001379: changing the `+0xe4` initialization to `0xdeadbeef` is detected by `NxPhysicsObjectLayoutTests` (`mesh candidate ok=0`, aggregate `mismatches=2`); restored output has zero mismatches and digest `422a1f78`. Phase 5 advances to 178 closed / 27 deferred; full gate passes at 2,565/2,565 assertions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-mesh-constructor-001379.md`.

Task M2-wall-dtor-000042: changing the interface-wall vtable to `0x101043d4` is caught by the actorctor test (`mismatches=1`); the restored `0x101043d0` table matches digest `19f4915a` with zero mismatches. Phase 5 advances to 179 closed / 26 deferred; full gate passes at 2,565/2,565 assertions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-wall-dtor-000042.md`.

Task M2-actor-ctor-tail-000044: changing the final vtable store to `0x10104534` is caught by the actorctor test (`mismatches=1`); the restored `0x10104530` table matches digest `19f4915a` with zero mismatches. Phase 5 advances to 180 closed / 25 deferred; full gate passes at 2,565/2,565 assertions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-actor-ctor-000044.md`.

Task M2-actor-dtor-000118: changing the final wall-vtable store to `0x101043d4` breaks both direct and +8-adjustor destructor observations (`mismatches=1`); restoring `0x101043d0` returns digest `19f4915a` with zero mismatches. Phase 5 advances to 181 closed / 24 deferred; full gate passes at 2,565/2,565 assertions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-actor-dtor-000118.md`.

Task M2-actor-dtor-adjustor-000116: suppressing flags forwarding prevents the adjusted flag-1 free (`adjFreed=0; mismatches=1`); restoring flags returns digest `19f4915a` with zero mismatches. Phase 5 advances to 182 closed / 23 deferred; full gate passes at 2,565/2,565 assertions. Evidence: `docs/reconstruction/novodex-physics/evidence/phase5-actor-dtor-thunk-000116.md`.


Continuation — Fluid emitter global-pose rows (`phys_fn_003559` / `003561`, 2026-10-12).

Both rows now call the `EmitterGetGlobalPose` backend callback with the oracle’s context, fluid, and emitter handles, transpose the 3x3 rotation words, and preserve translation. The direct pinned-oracle differential covers the cached-pose return and caller-output return, callback arguments, all 12 words, untouched output tail, and x86 stack balance. A rotation-index mutant is rejected in both cases (`mismatches=2`); restored control reports zero mismatches. Fresh full Phase 5 passes at 2,665/2,664 coverage assertions and Phase 7 passes at 1,692/1,691. Both full logs are under `docs/reconstruction/novodex-physics/evidence/raw/phase7-fluid-emitter-global-pose-003559-003561/`. Inventory validation reports 126 closed / 435 deferred Phase 7 functions and zero unexplained rows. Public headers remain unchanged.


### Continuation — Fluid emitter local aggregate copy rows (2026-10-12)

Rows `phys_fn_003563`, `phys_fn_003565`, and `phys_fn_003567` copy the emitter local pose, position, and orientation to their aggregate output buffers. The registered pinned-oracle ABI test compares the test-side candidate and loaded DLL, including return pointers and x86 stack balance. Independent one-dword source-offset mutations are each caught on both candidate paths (`mismatches=1`); restored outputs are exact. Fresh Phase 5 passes at 2,666/2,665 and Phase 7 at 1,693/1,692 coverage assertions. Inventory validation reports 129 closed / 432 deferred Phase 7 functions, 6,338 functions, 5,138 data objects, and zero unexplained rows. No public headers changed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-local-copy-003563-003567.md`.

### Continuation — integrate box/mesh dispatch and fluid reconstruction (`2026-10-14`)

Integrated the reviewed `codex/phase5-box-mesh-dispatch` branch on the current main baseline. It adds box/triangle-mesh contact and overlap dispatch, public fluid `addParticles` behavior, manager-growth allocation-failure handling, and registered standalone fixtures. Inventory and closure artifacts were reconciled by work-unit ID; only `phys_fn_003710` was newly closed by the branch's mutation-falsified public differential, while incomplete internal fluid rows remain open. No public Physics headers changed.

Validation on the integration branch: reconstruction tooling suite passed (829 tests and 740 subtests); focused fluid targets and `NxPhysicsSimulationTests` passed exact oracle/candidate differentials; fresh Phase 7 passed at 1,777/1,777 coverage assertions; fresh Phase 5 passed at 2,708/2,708. Full DLL reconstruction remains active.

### Continuation — physics worktree audit and mainline reconciliation (`2026-10-14`)

The focused box/mesh dispatch branch was merged and fast-forwarded as `cc543f86`; Phase 5 and Phase 7 passed at 2,708/2,708 and 1,777/1,777 coverage assertions. The older focused fluid-manager-dirty and fluid-release branches were audited against the resulting mainline. Their substantive work is already present with newer fixture/gate coverage: dirty refresh (`phys_fn_003630`), attached-body substep (`phys_fn_003495`), emitter guards (`phys_fn_003601`), PMap rows (`phys_fn_002047`, `phys_fn_002049`), and revolute destructor (`phys_fn_004395`) are represented by mainline code, evidence, and closed ledgers. Their old commits conflict with later source and generated fixtures; replaying them would reintroduce stale snapshots without adding the verified behavior.

The separate `codex/nxphysics-portable` line has 39 unique commits and is 359 commits behind main. It changes the architecture and build/source inventory broadly, so it is not a safe wholesale merge into the Windows x86 Unreal DLL target. Keep it as a separate porting track; audit candidate functionality individually after the current mainline testing path. The mainline DLL reconstruction remains active and public Physics headers remain untouched.

### Continuation — Viewer regression after box/mesh integration (`2026-10-14`)

Built the CMake Viewer and ViewerSceneTests against the integrated DLL, then ran the complete registered Viewer selection: all 39 checked-in scene demos, ViewerSceneTests, ViewerPhysicsStep, and ViewerPhysicsContact. CTest reports 43/43 successful selections: 38 passed and five expected exact-signature skips (CowPile, PMapTest 8/10/12, and TruckDemo, whose known baseline blockers are pinned in Viewer/CMakeLists.txt). The raw run is archived at `docs/reconstruction/novodex-physics/evidence/raw/viewer-all-scenes-after-box-mesh-20261014/viewer-ctest.log`. This is standalone Viewer coverage; Unreal consumer testing remains after the standalone milestones. Full DLL reconstruction remains active.

### Continuation — ParticleArrayGrow failure ordering (`phys_fn_003503`, 2026-10-14)

Added a registered standalone public-DLL differential for a zero-capacity ParticleArray whose ParticleArrayGrow callback fails during `NxFluid::addParticles`. Oracle and candidate both fault at record initialization with `c0000005`, preserve count zero and both manager arrays, and match exactly. A no-op grow-call mutant is rejected (`stdout_delta=2`, candidate exit 1); the restored control passes. The fresh full Phase 7 gate passes at 1,778/1,778 coverage assertions; the reconstruction tooling suite passes all 814 tests. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-array-grow-failure-003503-20261014.md` and archived transcripts. This is one branch of `phys_fn_003503`; the row remains open. Public Physics headers are unchanged.

### Continuation — ParticleArray append behavior (`phys_fn_003503`, 2026-10-14)

Added a registered public DLL case that creates a fluid with two initial particles and then adds two more. The oracle appends record data at the existing count offset, preserves the first records, and leaves the stored count at the new batch size (2); the candidate matches exactly. A mutation storing the cumulative count (4) is rejected (`stdout_delta=2`, candidate exit 1). The complete Phase 7 gate passes at 1,779/1,779, and the tooling suite passes 814 tests. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-particle-array-append-003503-20261014.md` and its raw transcripts. No product change was needed; `phys_fn_003503` remains open for its remaining branches.

### Continuation — fluid surface-mesh constructor warning (`phys_fn_003503`, 2026-10-14)

IDA showed the `NpFluid` constructor calling its optional surface-mesh constructor when `NxFluidDesc::surfaceMeshDesc` is present. A valid descriptor with the mesh feature unlicensed reports the oracle's warning at `Fluid.cpp:280` and still creates the fluid; the candidate now matches this behavior. The registered simulation differential passes exactly, and suppressing the warning is detected. Fresh Phase 7 passes at 1,779/1,779 and all 814 tooling tests pass. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-surface-mesh-constructor-warning-003503-20261014.md`. The licensed mesh constructor/backend path remains open, so `phys_fn_003503` is not closed.

### Continuation — public fluid surface-mesh invalid descriptor (`phys_fn_003704`, 2026-10-17)

IDA confirms that the invalid-descriptor path reports `NXE_INVALID_PARAMETER` in the internal initializer, returns null, then faults when the public wrapper unconditionally reads the returned manager. The candidate now preserves this ordering. The registered simulation fixture contains the fault in a child process and compares the pinned fault code, reach marker, and return marker. A safe-null mutation is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=82`); restored output is exact. Fresh Phase 5/6/7 gates pass at 2,835/2,834, 1,720/1,719, and 2,968/2,967 coverage assertions. The first Phase 6 run had an allocator-reuse mismatch; a focused retry and complete gate rerun passed. This proves one additional arm only and does not close the function. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-public-surface-mesh-invalid-003704.md` and its raw transcript archive.

### Continuation — implicit-mesh lock-base deleting destructor (`phys_fn_003864`, 2026-10-17)

The compiler-generated scalar deleting destructor for `NpImplicitMeshLockBase` is now covered by a candidate-DLL differential. It verifies the standalone base vptr, both seeded lock links, returned object, x86 stack balance, and flag-one deallocation through each DLL allocator. A mutation clearing `mWriteLockLink` is rejected (`oracle_links=1`, `candidate_links=0`, `mismatches=1`); the restored differential reports zero mismatches. Fresh Phase 5/6/7 gates pass at 2,836/2,836, 1,720/1,720, and 2,969/2,969 assertions. The Phase 7 ledger advances to 410 closed / 150 deferred. Public Physics headers are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-implicit-mesh-lock-base-deleting-dtor-003864.md` and `docs/reconstruction/novodex-physics/evidence/raw/phase7-implicit-mesh-lock-base-deleting-dtor-003864-20261017/`.

The compiler-generated `NpFluidEmitter` secondary scalar-deleting thunk (`phys_fn_003862`) is now checked through its secondary vtable for both delete flags. The differential checks adjustment effects, both base vptrs, seeded words, return identity, stack balance, and frees. The `sub ecx,8` to `sub ecx,4` mutant is rejected with four mismatches; the restored candidate hash is exact. Phase 7 advances to 411 closed / 149 deferred. Public Physics headers are unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-secondary-thunk-003862.md`.

The `NpFluidEmitter` scalar deleting destructor (`phys_fn_003794`) is now independently mutation-falsified through the secondary-vtable fixture. Replacing its primary-base vptr immediate is caught (`candidate_primary=0`, `mismatches=1`), and the candidate DLL is restored exactly. Phase 7 advances to 412 closed / 148 deferred; public Physics headers remain unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-deleting-dtor-003794.md`.

The inline `NxFluidEmitter` base deleting destructor (`phys_fn_003790`) now has a direct oracle/candidate differential for nondeleting and deleting flags, checking base-vptr installation, return, stack, and frees. Its base-vptr mutation is rejected with one mismatch; the restored DLL is byte-identical. Phase 7 advances to 413 closed / 147 deferred. Public Physics headers remain unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-base-deleting-dtor-003790.md`.

The `NpFluidEmitter` world-pose setters (`phys_fn_003798/003800/003802`) are now independently mutation-falsified by the oracle/candidate-source/candidate-DLL fixture. Immediate-return mutants are caught with 2, 1, and 1 mismatches; the restored DLL hash is exact. Phase 7 advances to 416 closed / 144 deferred. Public Physics headers remain unchanged. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-world-pose-setters-003798-003800-003802.md`.


Task M7-fluid-emitter-pose-setters (active): TDD red exposed missing full backend-pose cache refresh in rows 003798/003800/003802/003810/003812/003814; adding the `+0x48` cache refresh after backend publication makes the oracle/source/DLL full-state fixture exact. Six candidate-only no-op mutations are rejected 2/1/1/1/1/1; candidate DLL restored to `c9ae7d820bf51329364475698fc166f15b897bc709154fe6f2442354a61f2901`. Phase 5 passed 2845/2845 and Phase 7 passed 2978/2978 after serial clean configure/build. The reconstruction-tool suite passes 817 tests; inventory validation reports 6,338 functions, 5,138 data objects, zero unexplained; public headers pass all 80 hashes.
