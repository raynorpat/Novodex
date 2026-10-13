# `NxFluid::setStiffness` reconstruction (`phys_fn_003718`)

The public `NxFluid::setStiffness` slot now follows the pinned DLL at RVA `0x0008b680`: it reports the unavailable-feature warning when the stiffness license bit is absent, acquires the Scene write lock, enforces the shared callback reentry guard, and forwards the backend manager, fluid, and scalar to `FluidSetStiffness`. The implementation is in `Physics/src/Scene.cpp`; public headers are unchanged.

The simulation fixture uses the licensed product-465 path and checks scalar bits `0x3f200000`, one callback, the correct manager/fluid arguments, and zero diagnostics. A temporary no-forward mutation was rejected by the registered `NxPhysicsSimulationTests` staged-pair differential: oracle exit 0, candidate exit 1, `stdout_delta=52`. Restored control: both exits 0, `stdout_delta=0`, exact stderr.

Fresh full phase gates passed with the new marker required: Phase 5 `2,686/2,686`, Phase 6 `1,620/1,620`, Phase 7 `1,728/1,728`. The Phase 5/6/7 gate transcripts and mutation/build logs are in `evidence/raw/phase7-npfluid-set-stiffness-003718/`.
