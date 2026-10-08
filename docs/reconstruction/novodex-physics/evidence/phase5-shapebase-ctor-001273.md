# Phase 5: `ShapeBase` constructor (`phys_fn_001273`)

The registered `NxPhysicsObjectLayoutTests` pinned-oracle differential invokes the oracle constructor and the candidate C++ constructor on equivalent poisoned 0xe0-byte objects with a null owner and marked second argument. It folds all non-pointer words, separately checks the key fields, and asserts the prunable owner self-link.

- Baseline: oracle digest `de5f5000`; candidate `shapebase candidate ok=1 digest=de5f5000`; total layout mismatches `0`.
- Mutation: change `mHalfwordDE` from `8` to `9`. The candidate digest becomes `0461ca69`, `shapebase candidate ok=0`, and the registered differential reports `mismatches=15`.
- Restored control: candidate digest `de5f5000`, `shapebase candidate ok=1`, total mismatches `0`, and `layout result=differential-pass`.

The mutation was built through CMake as `NxPhysicsObjectLayoutTests`; source was restored before the control build. Logs: `build/phase5-shapebase-ctor-baseline.log`, `build/phase5-shapebase-ctor-mutant.log`, and `build/phase5-shapebase-ctor-restored.log`.
