# Scene body removal row 000632 (`phys_fn_000632`)

Replacing only `nxSceneRemoveBody` with a no-op is rejected by two registered Phase 7 staged-pair tests: `NxPhysicsSimulationTests` crashes the candidate (`candidate_exit=-1073741819`, `stdout_delta=3991`), and `NxPhysicsJointStagedPairTests` also crashes it (`candidate_exit=-1073741819`, `stdout_delta=2459`); both have exact stderr. Restoring `Scene.cpp` byte-for-byte returns exact output on both targets (`both exits=0`, `stdout_delta=0`, `stderr_exact=True`).

The full Phase 7 gate passed after restoration at 1,975/1,975 assertions. The mutation, both focused targets, restored controls, and gate transcript are preserved in `evidence/raw/phase7-scene-remove-body-000632-20261014/`. No public headers or production implementation sources changed.
