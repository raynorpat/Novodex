# `nxMaskedFourSlotLoop4868` mutation proof

Date: 2026-10-14
Row: `phys_fn_004868` at RVA `0x000b5410`
Gate: registered `NxPhysicsSceneRaycastTests` staged-pair differential (Phase 7)

Skipping the four selected-pruner slot calls while preserving the helper’s constant return value of 1 was rejected by the registered differential: both processes exited 0, `stdout_delta=5545`, and stderr matched exactly. The restored build matched the oracle exactly (`stdout_delta=0`, exact stderr).

The source label was also corrected: RVA `0xB5410` is census row `phys_fn_004868`; `phys_fn_004865` is padding at RVA `0xB53B1`. Raw mutant and restored runner logs are compressed under `evidence/raw/phase7-aabb-pruner-loop-004868-20261014/`.
