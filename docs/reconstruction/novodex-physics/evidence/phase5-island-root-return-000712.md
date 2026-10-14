# Island root lookup and path compression (`phys_fn_000712`)

The Phase 2-owned row at RVA `0x15d30` is `Row000712Fixture::row000712`. Its recorded driving phases are 5 and 7. Phase 5 now discharges the row through the registered `NxPhysicsSimulationTests` target.

## Mutation proof

Replacing the final root-pointer return with `return 0` in a throwaway source mutation and rebuilding through CMake (`NxPhysics` and `NxPhysicsSimulationTests`, Release, Win32) is rejected by the Phase 5 staged-pair differential: `oracle_exit=0`, `candidate_exit=-1073741819 (0xC0000005)`, `stdout_delta=771`, and exact stderr.

After restoring and rebuilding the path-compression helper, the simulation differential passes exactly (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). The full Phase 5 gate passes with 2,708/2,708 coverage assertions. No public Physics headers changed.

Raw mutation/control and gate logs are archived under `evidence/raw/phase5-island-root-return-000712-20261014/`.
