# Sleep-group union and member-list merge (`phys_fn_000718`)

The row at RVA `0x15fe0` is `Row000718Fixture::row000718`. It merges two sleep-group roots, splices member lists, updates the rank and combines each group's sleep thresholds and sleep parameter.

## Mutation proof

A throwaway source mutation returning before the merge was rebuilt through CMake (`NxPhysics` and `NxPhysicsSimulationTests`, Release, Win32). The registered Phase 7 staged-pair simulation differential rejected the mutation with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=4158`, and exact stderr.

After restoring and rebuilding the sleep-group helper, the differential passed exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). No public Physics headers changed.

The fresh full Phase 7 gate and raw mutation/control logs are archived under `evidence/raw/phase7-sleep-group-union-000718-20261014/`.
