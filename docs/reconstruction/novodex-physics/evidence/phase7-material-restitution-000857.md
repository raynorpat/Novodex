# Material-pair restitution row 000857 (`phys_fn_000857`)

The registered `NxPhysicsSimulationTests` contact cases exercise `cpmRestitution0857` through material-pair response. Replacing only this row with a no-op caused the candidate to access-violate (`candidate_exit=-1073741819`) while the oracle exited 0; the staged differential reported `stdout_delta=3954` with exact stderr. Restoring `ContactPairManager.cpp` byte-for-byte returned an exact differential (`both exits=0`, `stdout_delta=0`, `stderr_exact=True`). The mutation and restored candidate DLL hashes are recorded in the raw packet.

The full Phase 7 gate passed after restoration at 1,975/1,975 coverage assertions. `NxPhysicsSimulationTests` is already registered in Phase 7. The pre-merge gate and both focused differentials are preserved in `evidence/raw/phase7-material-restitution-000857-20261014/`. No public headers or production implementation sources changed.
