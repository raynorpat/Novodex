# Island support-body copy-back (`phys_fn_000708`)

The row at RVA `0x15c20` is `Row000708Fixture::row000708`. It copies the completed `JointSupportBody` vectors back to the island body record and marks the saved velocity state valid.

## Mutation proof

A throwaway source mutation returning before the copy-back stores was rebuilt through CMake (`NxPhysics` and `NxPhysicsSimulationTests`, Release, Win32). The registered Phase 7 staged-pair simulation differential rejected the candidate with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=6986`, and exact stderr.

After restoring and rebuilding the original copy-back, the differential passed with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and exact stderr. No public Physics headers changed.

The fresh full Phase 7 gate and raw mutation/control logs are archived under `evidence/raw/phase7-island-support-copy-back-000708-20261014/`.
