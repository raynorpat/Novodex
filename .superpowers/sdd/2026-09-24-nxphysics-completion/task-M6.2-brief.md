# Task M6.2 — scene joint release

## Contract

Close `phys_fn_000653` (`NxSceneInternal::releaseJoint`, RVA `0x13760`) using the already registered `NxPhysicsJointStagedPairTests` differential. Preserve the current re-entry guard, joint unlink/deletion, scene joint count update, iterator reset, and guard cleanup. Public headers remain unchanged.

## Proof loop

1. Build a throwaway `git archive` of the packet HEAD with the ignored pinned oracle assets required by the staged-pair runner, then capture the focused joint staged-pair baseline.
2. Insert an immediate return at the start of `releaseJoint`, rebuild, and require the registered differential to reject it.
3. Restore the archive source byte-for-byte, rebuild, and require exact oracle/candidate output.
4. Run the Phase 7 gate and inventory, backlog, work-unit, gate-registry, immutable-header, and diff checks.
5. Record candidate hashes and results; close only `phys_fn_000653`.

## Scope boundaries

No changes to public headers or joint implementations. This proof establishes the existing scene release behavior; remaining simulation state transitions and solver paths stay open under M6.
