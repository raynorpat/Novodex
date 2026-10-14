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

Raw differential, mutation, Phase 7, post-merge Phase 5, and initial
integration-failure logs are archived under
`evidence/raw/phase7-fluid-licensed-surface-mesh-003503-20261014/`.

This validates the constructor, callback arguments, `userData`, and destruction
branch. The public `NxImplicitMesh` property and particle methods still need
their own oracle differentials, including lock/reentry behavior. Therefore
`phys_fn_003503` remains open and the full DLL reconstruction remains active.
No public Physics headers were changed.
