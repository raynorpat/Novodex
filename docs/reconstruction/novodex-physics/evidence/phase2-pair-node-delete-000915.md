# Pair-node deletion (`phys_fn_000915`)

The registered `NxPhysicsCollisionTests` fixture previously compared oracle RVA `0x00020020` with `NxScenePrunerNodeDestroy`, a second implementation in `Scene.cpp`. The inventory maps this row to `cpmDeletePairNode0915` in `ContactPairManager.cpp`, so the old fixture did not falsify the mapped candidate function.

The fixture now calls `cpmDeletePairNode0915` directly. Its non-null case checks the list head/tail and node links after unlinking, allocator frees, and the full fixture digest; the null case checks the no-op. The restored direct candidate matches the oracle in both cases (`mismatches=0`, oracle digest `4d25767f9dce13f5`, two frees; differential `stdout_delta=0`, `stderr_exact=True`).

For the mutation control, the candidate unlink call was omitted while the same test executable and gate were rebuilt. The non-null case retained the freed node as the list head; the gate reported `mismatches=1` and failed. Restoring the call returned to the exact control. The raw baseline, mutant, and restored transcripts and their hashes are in [the evidence archive](raw/phase2-pair-node-delete-000915-20261018/manifest.json).
