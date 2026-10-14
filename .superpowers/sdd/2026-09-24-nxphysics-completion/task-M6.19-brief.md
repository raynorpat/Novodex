# Task M6.19 brief — island sleep/wake state update (`phys_fn_000732`)

Plan: `docs/superpowers/plans/2026-09-24-nxphysics-completion.md`
Base: `108fcb9084c05d48fd66fcf25e8dcdc3e6d85743`

## Contract

Falsify the reconstructed `Row000732Fixture::row000732` at RVA `0x16860`, which calls the island sleep/wake updater (`phys_fn_000714`) and stores/restores the body-step velocity state under the record flags. Use the existing registered `NxPhysicsSimulationTests` staged-pair fixture; preserve the current implementation unless a pinned oracle mismatch is established. Public headers and CMake remain immutable.

## Proof loop

1. Build a clean Win32 Release archive of the base commit and capture the focused simulation differential.
2. Replace only `row000732` with an immediate return in the throwaway archive, rebuild, and require the registered differential to detect the changed sleep/wake state.
3. Restore source byte-for-byte, rebuild, and require exact differential output.
4. Run the Phase 7 gate, inventory/backlog and immutable-header checks, relevant Python tests, and `git diff --check`.
5. Retain the mutation patch, candidate/oracle hashes, raw logs, and update the closure/inventory only if the output proves this row's behavior.

## Scope boundary

This packet covers only row 000732's exercised sleep/wake state transition. It does not close 000714 or the rest of the simulation pipeline; Phase 8 terminal closure remains open.
