# Phase 7: licensed implicit surface-mesh construction (`phys_fn_003503`)

The licensed `NxFluid` constructor path now creates the optional implicit surface
mesh when the descriptor supplies one and the installed feature flags include
the mesh feature. The candidate allocates the measured internal manager record,
copies the 0x30-byte `NxMeshData`, forwards the backend fluid/manager and all
five descriptor settings to `ImplicitFluidMeshCreate`, constructs the public
`NpImplicitMesh`, preserves `userData`, and registers the descriptor name. The
matching destruction path calls `ImplicitFluidMeshDestroy`, removes the name
mapping, deletes the public wrapper, and releases the internal manager record.
The unlicensed-warning behavior remains covered by its earlier differential.

`NxPhysicsSimulationTests` now exercises the licensed descriptor through the
public DLL: it checks the created mesh, `userData`, callback arguments, and
backend destroy callback after `releaseSurfaceMesh`. The registered
oracle/candidate differential is exact (`oracle_exit=0`, `candidate_exit=0`,
`stdout_delta=0`, exact stderr). A mutation that skips the create callback is
rejected (`callback=0`, `matched=0`, candidate exit 1).

The full Phase 7 gate passes at 1,779/1,779 coverage assertions. The
reconstruction tooling suite passes 829 tests and 740 subtests. Adding the new
wrapper implementation also exposed that `NxPhysicsInternalTests` and
`NxPhysicsCollisionTests` compile `Scene.cpp` directly; both CMake source lists
now compile `ImplicitMesh.cpp` as well. Both targets build successfully.
After commit `5a679638` was fast-forwarded to `main`, the integrated Phase 5
gate also passed at 2,708/2,708 coverage assertions.

The follow-up public `NxFluid::createSurfaceMesh` entry (`phys_fn_003705`, IDA
`0x0008b250`) is now in the fluid vtable. It checks the feature license, then
takes the Scene write lock and reentry guard, validates the descriptor,
replaces an existing mesh, and attaches the owning fluid's read/write lock
links to the returned wrapper. Those links are needed by the public mesh methods; the
constructor-created wrapper intentionally has no Scene lock links. The
registered differential calls the public entry, then checks initial and
updated radii, triangle size, smoothing count, blend warning/return, name
pointer round trips, and `NxMeshData` copy-out. Every callback receives valid
backend manager/fluid handles. The baseline with the vtable slot missing is
rejected (oracle exit 0, candidate access violation, `stdout_delta=77`); after
the implementation, oracle and candidate match exactly (`stdout_delta=0`,
exact stderr).

The same differential calls `setParticles` with default particle data, copies
a valid indexed `NxMeshData` into the mesh and reads it back, then passes an
invalid mesh-data wrapper. Both DLLs report the invalid-parameter error and
preserve the previous mesh data; the exact differential remains green.

On integrated mainline commit `d8d66901`, Phase 5 passes at 2,708/2,708 and
Phase 7 at 1,779/1,779. The Viewer sweep passes all 43 registered selections:
38 pass and five known baseline skips (CowPile, PMapTest 8/10/12, TruckDemo).
The internal and collision CMake targets that compile `Scene.cpp` directly
both build. The tooling suite remains green at 829 tests and 740 subtests.

After extending the registered public test with valid/invalid `setMeshData` and
`setParticles`, the focused differential remains exact and a fresh Phase 7 run
on mainline commit `a2304cae` passes again at 1,779/1,779. The updated
differential and gate logs are in
`evidence/raw/phase7-fluid-explicit-surface-create-003705-20261014/`.

The public setter differential now also reenters the same mesh setter once from
the backend callback. The oracle emits one reentry error, does not dispatch a
nested backend callback, and preserves the outer setter's value; the candidate
matches exactly (`stdout_delta=0`, exact stderr). The focused reentry
differential and a fresh Phase 7 gate on merged mainline commit `745a9c25`
pass; Phase 7 remains at 1,779/1,779. Their archived logs are in
`evidence/raw/phase7-fluid-explicit-surface-create-003705-20261014/`.

Raw differential, mutation, Phase 7, post-merge Phase 5, and initial
integration-failure logs are archived under
`evidence/raw/phase7-fluid-licensed-surface-mesh-003503-20261014/`.
The public-entry red baseline, exact differential, gates, Viewer sweep, and
direct-target build are archived under
`evidence/raw/phase7-fluid-explicit-surface-create-003705-20261014/`.

This validates the constructor/destruction paths, valid-descriptor public
creation, property getters/setters, `setParticles`, valid/invalid
`setMeshData`, and callback reentry. A registered constructor test now fails the
measured 0x3c-byte surface-mesh manager allocation while creating a licensed
fluid. Both DLLs still create and release the fluid, report no error, and make
no implicit-mesh backend callbacks; the focused differential is exact. A
mutation removing the allocation-null guard faults only the candidate and is
rejected. The focused test and fresh Phase 7 gate pass at 1,779/1,779; archived
transcripts are in `evidence/raw/phase7-fluid-licensed-surface-mesh-003503-20261014/`.

IDA shows that `phys_fn_003705` indexes the manager returned by the internal
constructor without a null check. Invalid descriptors therefore remain open
for further oracle validation, as do lock contention and later wrapper
allocation failure. Keep
`phys_fn_003503` and `phys_fn_003705` open; the full DLL reconstruction remains
active. No public Physics headers were changed.
