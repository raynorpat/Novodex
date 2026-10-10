# Task M6.1 — simulation frame scheduler row

## Contract

Close `phys_fn_000659` (`NxSceneInternal::simulateFrame`, RVA `0x13c40`) with a registered oracle/candidate staged-pair differential that falsifies a whole-row semantic mutation. Keep public headers unchanged. Preserve the current scheduler implementation and its exact x87 control-word behavior.

## Proof loop

1. Create a throwaway `git archive` from the packet HEAD, copy in the ignored pinned oracle assets needed by the runner, build a clean x86 Release candidate, and capture the focused `NxPhysicsSimulationTests` staged-pair baseline.
2. Replace only `simulateFrame` with an immediate return inside the throwaway archive, rebuild, and require the registered differential to reject it with nonzero output or a candidate assertion/exit failure.
3. Restore the source byte-for-byte, rebuild, and require exact control output.
4. Run the Phase 7 differential gate, inventory/closure validation, relevant metadata tests, public-header validation, and `git diff --check`.
5. Record hashes and command outcomes; close the inventory row only after all proof steps pass.

## Scope boundaries

No public-header changes, no unrelated scheduler or extension work, and no closure claim for rows `phys_fn_003630` or `phys_fn_003632` beyond their separately tracked status. The project completion plan remains open after this packet.
