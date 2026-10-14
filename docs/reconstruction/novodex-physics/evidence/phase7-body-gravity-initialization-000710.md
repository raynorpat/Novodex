# Body-step gravity initialization (`phys_fn_000710`)

The row at RVA `0x15cb0` is `Row000710Fixture::row000710`. It initializes the body record's gravity contribution according to the gravity-disabled flag and clears the auxiliary acceleration vector words.

## Mutation proof

A throwaway source mutation returning before these stores was rebuilt with CMake (`NxPhysics` and `NxPhysicsSimulationTests`, Release, Win32). The registered Phase 7 staged-pair simulation differential rejected it: `oracle_exit=0`, `candidate_exit=1`, `stdout_delta=6870`.

After restoring and rebuilding the original initialization helper, the differential passed exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). No public Physics headers changed.

The fresh full Phase 7 gate and raw mutation/control logs are archived under `evidence/raw/phase7-body-gravity-initialization-000710-20261014/`.
