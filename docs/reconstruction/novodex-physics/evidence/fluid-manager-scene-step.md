# Disabled FluidManager scene-step integration (2026-10-06)

Oracle `phys_fn_000659` calls `phys_fn_003630` after each completed substep
when the Scene has a FluidManager at `+0x61c`, then calls
`phys_fn_003632` once after the scene's substep loop. On the pinned engine
installation, the fluid extension is unavailable and the manager has its
backend-available byte at `+0x2b` clear. Those calls still emit the disabled
backend warnings.

The regression creates a public scene, calls `createFluid` to establish the
disabled manager, and sets the manager's extension-dirty byte at `+0x28` to
zero on both sides. That byte is conditional storage the constructor leaves
uninitialized; pinning it to zero selects the warning-only branch and avoids
calling absent extension functions. Two fixed simulate/check/fetch frames
should produce three warnings: `stepFluids` once per frame and
`generateSurfaceMeshes` once after the loop. The first paired run was RED:
oracle reported three warnings and candidate reported zero. After adding the
calls at the matching scheduler boundaries, the normalized output is exact
(`stdout_delta=0`, `stderr_exact=True`; `build/FluidGate/fluid-step-green.log`).

The full Phase 5 gate passes at 2,042/2,042
(`build/FluidGate/phase5-fluid-step.log`), Phase 7 passes at 1,327/1,327
(`build/FluidGate/phase7-fluid-step.log`), and the full Viewer CTest selection
passes all 48 selections; 34 scene demos run and five known pinned-oracle
failures are skipped by exact registered signatures
(`build/FluidGate/viewer-scenes-after-fluid-step.log`).

Inventory rows `phys_fn_000659` and `phys_fn_000655` now record reconstructed
scene control flow, with whole-row mutation falsification still open. Rows
`phys_fn_003630` and `phys_fn_003632` remain discovered with partial evidence:
the enabled extension's static-collision update, live-fluid iteration, and
surface-mesh dispatch are not covered. Public Physics headers are unchanged.
