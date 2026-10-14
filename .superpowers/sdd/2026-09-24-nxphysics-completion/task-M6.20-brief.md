# Task M6.20 brief — dirty island rebuild dispatch (`phys_fn_000720`)

Plan: `docs/superpowers/plans/2026-09-24-nxphysics-completion.md`
Base: `fe81254c` (post-merge sleep-state evidence)

## Contract

Falsify reconstructed `Row000720Fixture::row000720` at RVA `0x16100`. It compresses the joint-island root and, when the root dirty flag is set, calls the island-object rebuild helper `nxBodyIslandRebuild004172`. Use the existing registered Phase 7 `NxPhysicsSimulationTests` staged-pair fixture. Keep implementation and public headers unchanged unless the pinned oracle disproves the source.

## Proof loop

1. Build a clean Win32 Release archive and capture the exact baseline simulation differential.
2. Replace only `row000720` with an immediate return in a throwaway archive and require the registered differential to reject it. If the output delta is zero, do not close the row; inspect the public scenarios for a dirty-island path or select a different reachable row.
3. Restore source byte-for-byte, rebuild, and require exact differential output.
4. Run Phase 7, inventory/backlog, immutable-header, tool tests, and `git diff --check`.
5. Retain row-specific patch, source/DLL hashes, raw logs, and update the closure only when the mutation is measured.

## Scope boundary

This packet covers row 000720 only. It does not re-close helper 004172 or any other island rows; Phase 7 and full-DLL reconstruction remain open.


## Result

No closure: the immediate-return mutant produced stdout_delta=0 and exact stderr in NxPhysicsSimulationTests. The archive source was restored byte-for-byte (SHA-256 4c02a1a7e5d7321d9bbead4b5c4b7e7cd5efdb4a1956d28d490891395734995a). The existing fixture does not mark the island dirty, so row 000720 stays open pending a specific dirty-island scenario.
