# Phase 7 `phys_fn_003613` / `003614`: global pose component setters

IDA decompilation shows `003613` reading the current backend global pose,
preserving its 3x3 orientation, replacing translation with the requested
position, then delegating to `phys_fn_003611`. `003614` reads the same backend
pose, copies the requested orientation while preserving its translation, and
delegates to `003611`. The backend pose stores its matrix column-major, while
the `NxMat34` passed to the global-pose setter is row-major.

`NpFluidEmitter::setGlobalPosition` and `setGlobalOrientation` now read the
backend pose and transpose its rotation into the `NxMat34` layout before
replacing the requested component. The registered `NxPhysicsFluidEmitterAbiTests`
fixture gives local and backend poses deliberately different values, then
compares the oracle, linked candidate and rebuilt candidate DLL. It checks
both resulting local poses, the `EmitterSetGlobalPose` callback payload and
x86 stack balance.

Replacing the world-pose read with the local-pose getter in the position
setter is rejected (the position case mismatches); the same mutation in the
orientation setter is rejected in its orientation case. Restoring both reads
produces `cases=2 callback_checks=2 stack_checks=2 mismatches=0`. Raw mutant and
restored transcripts are under
`evidence/raw/phase7-fluid-global-components-003613-003614-20261016/`.

These rows cover unattached emitters; attached actor transforms remain covered
by the shared `phys_fn_003611` differential. Public Physics headers are
unchanged.
