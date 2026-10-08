# Phase 5 — `phys_fn_000742` mass-energy helper

The registered `NxPhysicsObjectLayoutTests` fixture directly calls `phys_fn_000742` and actor slot 62. Its prior power-of-two values could hide association and contribution errors, so both the oracle and candidate records now use finite, non-binary values. The oracle and candidate report `4240985d` for both direct and slot calls. The expected oracle-side digests and coverage floor were updated; the clean Phase 5 run passes all registered targets at 2,311/2,311 assertions (`build/phase5-energy-order-gate-rerun.log`).

Mutation check: in the managed `nxphysics-m0-current` worktree, replacing the helper's `v194 * (m80 * m80)` contribution with zero makes the registered target report `actorsm2 candidate ok=0`, energy `422b41c8` instead of `4240985d`, `mismatches=1`, and exit 1 (`build/energy-x87-order-mutant.log`). Restoring `ObjectModel.cpp` from the worktree's original `HEAD`, rebuilding, and rerunning returns `actorsm2 candidate ok=1`, exact energy words, zero layout mismatches, and exit 0 (`build/energy-x87-order-restored.log`).

This closes the direct helper row `phys_fn_000742`; Phase 5 advances to 126 closed / 79 deferred. The broader Phase 5 gate remains pending until every owned code row is closed. Public Physics headers were not changed.
