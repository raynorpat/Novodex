# Scene body insertion row 000630 (`phys_fn_000630`)

The registered `NxPhysicsSimulationTests` exercises dynamic body insertion into the Scene array. Replacing only `nxSceneAddBody` with a no-op causes the candidate to fail (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=6906`); restoring `Scene.cpp` byte-for-byte returns an exact differential (`both exits=0`, `stdout_delta=0`, `stderr_exact=True`).

The full Phase 7 gate passed after restoration at 1,975/1,975 assertions. The row mutation, restored control, and full gate transcripts are preserved in `evidence/raw/phase7-scene-add-body-000630-20261014/`. No public headers or production implementation sources changed.
