# Scene joint removal row 000633 (`phys_fn_000633`)

Replacing only `NxSceneInternal::removeJoint` with an immediate no-op is rejected by the registered Phase 7 `NxPhysicsJointStagedPairTests` differential: the oracle exits 0, while the candidate access-violates (`candidate_exit=-1073741819`, `stdout_delta=3520`, `stderr_exact=True`). Restoring `Scene.cpp` byte-for-byte and forcing a fresh CMake rebuild returns an exact control (`both exits=0`, `stdout_delta=0`, `stderr_exact=True`).

The formal Phase 7 gate passes at 1,975/1,975 coverage assertions. The mutant, restored control, full gate transcript, mutation patch, and SHA-256 manifest are preserved in `evidence/raw/phase7-scene-remove-joint-000633-20261014/`. No public headers or production implementation sources changed.
