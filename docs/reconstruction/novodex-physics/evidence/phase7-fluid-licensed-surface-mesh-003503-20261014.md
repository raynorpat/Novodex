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

On integrated mainline commit `d8d66901`, Phase 5 passes at 2,708/2,708 and
Phase 7 at 1,779/1,779. The Viewer sweep passes all 43 registered selections:
38 pass and five known baseline skips (CowPile, PMapTest 8/10/12, TruckDemo).
The internal and collision CMake targets that compile `Scene.cpp` directly
both build. The tooling suite remains green at 829 tests and 740 subtests.

Raw differential, mutation, Phase 7, post-merge Phase 5, and initial
integration-failure logs are archived under
`evidence/raw/phase7-fluid-licensed-surface-mesh-003503-20261014/`.
The public-entry red baseline, exact differential, gates, Viewer sweep, and
direct-target build are archived under
`evidence/raw/phase7-fluid-explicit-surface-create-003705-20261014/`.

This validates the constructor/destruction paths and the valid-descriptor
public creation plus property getter/setter paths. Invalid-descriptor and
callback-reentry branches, allocation failures, `setParticles`, and
`setMeshData` still need row-specific differentials. Keep `phys_fn_003503` and
`phys_fn_003705` open; the full DLL reconstruction remains active. No public
Physics headers were changed.
