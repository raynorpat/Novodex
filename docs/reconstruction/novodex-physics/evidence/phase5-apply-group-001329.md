# Phase 5: shape apply-group (`phys_fn_001329`)

The registered `NxPhysicsObjectLayoutTests` pinned-oracle `grouperr` fixture constructs a real sphere, applies invalid group `0xff` and then valid group `5`, and compares both the captured error report and the resulting +0xd8 group word / +0xc8 prunable mask.

- Baseline oracle/candidate digest: `b6f879ec`; candidate check passes and the full differential reports zero mismatches.
- Mutation: replace the prunable mask assignment `1u << ((mHalfwordD8 & 0xff) & 0x1f)` with zero. Candidate digest becomes `b17097cc`; `grouperr candidate ok=0`; full target reports `mismatches=1` and exits 1.
- Restored control: candidate digest returns to `b6f879ec`, candidate check passes, and the target reports `mismatches=0` (`layout result=differential-pass`).

Logs: `build/phase5-apply-group-baseline.log`, `build/phase5-apply-group-mutant.log`, and `build/phase5-apply-group-restored.log`.
