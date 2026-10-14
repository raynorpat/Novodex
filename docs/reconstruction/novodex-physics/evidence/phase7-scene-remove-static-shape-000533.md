# Static shape removal row 000533 (`phys_fn_000533`)

The registered `NxPhysicsSimulationTests` exercises static shape removal during scene and actor lifecycle. Replacing only `nxSceneRemoveStaticShape` with a no-op that returns true causes the candidate to access-violate (`candidate_exit=-1073741819`) while the oracle exits 0; the staged differential reports `stdout_delta=3953` with exact stderr. Restoring `Scene.cpp` byte-for-byte returns an exact differential (`both exits=0`, `stdout_delta=0`, `stderr_exact=True`).

The full Phase 7 gate passed after restoration at 1,975/1,975 assertions. The row-specific mutant, restored control, and full gate transcript are preserved in `evidence/raw/phase7-scene-remove-static-shape-000533-20261014/`. No public headers or production implementation sources changed.
