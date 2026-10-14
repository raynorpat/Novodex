# Island-root union and member-list merge (`phys_fn_000716`)

The row at RVA `0x15ec0` is `Row000716Fixture::row000716`. It joins two disjoint island roots by rank, splices their member lists, combines counts, and updates root-state flags.

## Mutation proof

A throwaway source mutation returning before the merge was rebuilt with CMake (`NxPhysics` and the simulation/joint targets, Release, Win32). The registered `NxPhysicsSimulationTests` differential rejected the candidate with `oracle_exit=0`, `candidate_exit=-1073741819 (0xC0000005)`, `stdout_delta=782`, and exact stderr. The related `NxPhysicsJointStagedPairTests` fixture remained exact, documenting its current coverage boundary.

After restoring and rebuilding the union helper, both simulation and joint staged-pair differentials passed exactly (`stdout_delta=0`, exact stderr). No public Physics headers changed.

The fresh full Phase 7 gate and raw mutation/control logs are archived under `evidence/raw/phase7-island-root-union-000716-20261014/`.
