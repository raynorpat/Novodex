# Public triangle-mesh creation wrapper (`phys_fn_000242`)

`NpPhysicsSDK::createTriangleMesh` already implemented the wrapper contract: acquire every live Scene's write lock, unwind earlier locks and report the pinned error if a lock is contended, create the internal mesh, release all locks, and return the internal mesh's public wrapper. Its lock-contention behavior was exercised by `NxPhysicsSimulationTests`; the remaining missing evidence was an isolated mutation of the public-handle return path.

The existing `NxPhysicsTriangleMeshApiTests` fixture exercises the wrapper through the actual staged DLL and requires a successful public mesh before running descriptor, PMap, and simulation cases. Replacing the wrapper's return with null is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=26`). Restoring the return yields exact output (`both exits=0`, `stdout_delta=0`, exact stderr). The focused differential and both build logs are archived under `evidence/raw/phase2-triangle-mesh-create-000242-20261014/`.

This closes the public wrapper row only. The internal `PhysicsSDK::createTriangleMesh` implementation and its dependent mesh construction/teardown rows remain separate open work. Public headers are unchanged.


The fresh Phase 2 gate passes at 7/7 coverage assertions; the immutable public-header check passes. Inventory validation reports Phase 2 at 67 closed / 76 deferred.
