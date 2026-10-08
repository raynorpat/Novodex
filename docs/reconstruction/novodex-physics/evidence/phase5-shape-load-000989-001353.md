# Phase 5 — capsule and sphere descriptor loads

The registered `NxPhysicsObjectLayoutTests` fixture directly drives both shape load rows and compares the candidate record against the pinned oracle. The baseline output is exact (`build/phase5-sphere-capsule-load-baseline.log`).

For `phys_fn_000989` (`CapsuleShape::nxCapsuleLoadFromDesc`), adding 1.0f to the radius immediately after the descriptor load is caught: `capload candidate ok=0`, `layout candidate mismatches=2` (`build/phase5-capsule-load-000989-mutant.log`). The restored candidate reports radius `3fc00000` and zero mismatches (`build/phase5-capsule-load-000989-restored.log`).

For `phys_fn_001353` (`SphereShape::nxSphereLoadFromDesc`), changing the stored radius from `r` to `r + 1.0f` is caught: `sphload candidate ok=0`, `layout candidate mismatches=2` (`build/phase5-sphere-load-001353-mutant.log`). Restoring the implementation reports radius `40200000` and zero mismatches (`build/phase5-sphere-load-001353-restored.log`).

This closes the capsule and sphere descriptor-load rows. Phase 5 now records 131 closed / 74 deferred functions. Production behavior and public Physics headers are unchanged.

Fresh Win32 Release Phase 5 passes all 18 staged targets and 2,311/2,311 assertions with both descriptor-load implementations restored (`build/phase5-shape-load-000989-001353-full-gate.log`).
