# Task M6.3 — scene joint insertion

## Contract

Close `phys_fn_000661` (`NxSceneInternal::addJoint`, RVA `0x13e00`) through the registered `NxPhysicsJointStagedPairTests` differential. Preserve duplicate-insertion diagnostics, joint-list insertion, pointer-array growth, and the final scene-owner write. Keep public headers unchanged.

## Proof loop

1. Verify the current `main` source is byte-identical to the last clean archive for implementation inputs and capture the focused staged-pair baseline.
2. In the throwaway archive, insert an immediate return at the start of `addJoint`, rebuild, and require the differential to reject the mutant.
3. Restore the archive source byte-for-byte, rebuild, and require exact control output.
4. Run the Phase 7 gate, inventory and generated-artifact checks, focused metadata tests, public-header validation, and `git diff --check`.
5. Record hashes and outcomes; close only `phys_fn_000661`.

## Scope boundaries

No public-header or joint-family changes. The packet proves scene insertion behavior and leaves the remaining solver, simulation-state, and callback paths open under M6.
