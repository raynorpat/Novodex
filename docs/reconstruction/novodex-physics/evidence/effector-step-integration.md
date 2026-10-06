# Effector step integration (2026-10-06)

`phys_fn_000655` (oracle RVA `0x000137e0`) walks the scene effector list after
island contact solving and dispatches vtable slot 2 on each entry. In the
reconstructed public scene step, this is `Effector::tick()`, which applies the
spring/damper force before post-step body velocity bookkeeping. This closes the
missing scene-to-effector step edge; it does not reconstruct every branch or
callee in the 475-byte oracle function.

The RED fixture uses one dynamic box and a world-anchor spring whose starting
distance is 2.0, relaxed distance 1.0, and stiffness 0.5. With zero gravity
and fixed 1/60-second substeps, the first-step x velocity remains zero on both
sides. Before the scene-loop fix, the second-step candidate velocity also
remained zero; the oracle produced `0x3e8e38e3`. After the fix, both sides
produce the same second-step velocity and exact normalized output.

Verification on 2026-10-06:

- Focused `NxPhysicsSimulationTests` paired differential: `stdout_delta=0`,
  `stderr_exact=True` (`build/FluidGate/effector-tick-green.log`).
- Full Phase 5 gate: passes all 13 targets at 2,042/2,042 assertions
  (`build/FluidGate/phase5-effector-tick.log`).
- Full Phase 7 gate: passes at 1,326/1,326 assertions
  (`build/FluidGate/phase7-effector-tick.log`).
- Full Viewer CTest selection: 48 selections pass; 34 scene demos run, five
  known pinned-oracle signature failures are skipped, and Viewer step/contact
  tests pass (`build/FluidGate/viewer-scenes-after-effector-tick.log`).

The inventory row remains `discovered`: complete row 000655 reconstruction,
all dynamic branches, and whole-function mutation closure are still open. The
public Physics headers are unchanged.
