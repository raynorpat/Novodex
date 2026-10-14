# Island sleep/wake group update (`phys_fn_000714`)

The row at RVA `0x15d70` is `Row000714Fixture::row000714`. It reconciles the sleep-group version, evaluates saved or live body velocities against sleep thresholds, advances the sleep timer, raises a wake when required, and clears the awake-state flag.

## Mutation proof

A throwaway source mutation returning before the update was rebuilt with CMake (`NxPhysics` and `NxPhysicsSimulationTests`, Release, Win32). The registered Phase 7 staged-pair simulation differential rejected it with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=5918`, and exact stderr.

After restoring and rebuilding the original update helper, the differential passed exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). No public Physics headers changed.

The fresh full Phase 7 gate and raw mutation/control logs are archived under `evidence/raw/phase7-island-sleep-wake-update-000714-20261014/`.
