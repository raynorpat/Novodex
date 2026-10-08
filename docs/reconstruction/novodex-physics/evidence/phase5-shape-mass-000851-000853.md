# Phase 5 — sphere and capsule mass-frame builders

The existing registered `NxPhysicsActorMassTests` fixture exercises centered and posed spheres and capsules, compounds, explicit density and mass scaling, and trigger exclusions. The oracle/candidate baseline matches exactly (`stdout_delta=0`, `stderr_exact=True`; `build/phase5-sphere-capsule-mass-baseline.log`).

For `phys_fn_000851` (`SphereShape::nxSphereComputeMassFrame`), changing the radius passed into `nxMassFrameBuildSphere` to `radius + 1.0f` is caught with `stdout_delta=16`; both processes exit 0 and stderr matches exactly (`build/phase5-sphere-mass-000851-mutant.log`). Restoring the call returns an exact differential (`build/phase5-sphere-mass-000851-restored.log`).

For `phys_fn_000853` (`CapsuleShape::nxCapsuleComputeMassFrame`), changing the radius passed into `nxMassFrameBuildCapsule` to `radius + 1.0f` is caught with `stdout_delta=18`; both processes exit 0 and stderr matches exactly (`build/phase5-capsule-mass-000853-mutant.log`). Restoring the call returns an exact differential (`build/phase5-capsule-mass-000853-restored.log`).

This closes two shape-specific Phase 5 rows. The full Phase 5 gate remains pending with 129 closed / 76 deferred functions. Production implementation and public Physics headers are unchanged.

Fresh Win32 Release Phase 5 passes all 18 staged targets and 2,311/2,311 assertions with both restored shape mass routines (`build/phase5-shape-mass-000851-000853-full-gate.log`).
