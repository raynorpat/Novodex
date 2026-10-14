# Triangle-mesh release lock refusal (`phys_fn_000244`)

The Phase 2 `NxPhysicsTriangleMeshReleaseLockTests` fixture creates a real SDK, two Scenes and a mesh, then has a worker thread hold the second Scene write lock while the SDK releases the mesh. The call must return without blocking, report exactly one `NXE_INVALID_OPERATION` at line 127 with the pinned deadlock message, and preserve the mesh so the unlocked release can succeed afterward. Its second-Scene setup marker is independently registered in the gate.

Making the `NpPhysicsSDK::releaseTriangleMesh` lock-failure diagnostic a no-op is detected by the target (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=5`). Restoring the implementation returns an exact staged-pair differential (`both exits=0`, `stdout_delta=0`, exact stderr). The fixture requires second-Scene, locked-unwind and unlocked-release markers. Raw transcripts and build logs are under `evidence/raw/phase2-triangle-mesh-release-lock-000244-20261014/`.

This closes the public release-wrapper lock and success paths. The internal `PhysicsSDK::releaseTriangleMesh` implementation is a separate row. Public headers are unchanged.
