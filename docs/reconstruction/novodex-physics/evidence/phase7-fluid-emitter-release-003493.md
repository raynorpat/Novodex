# Fluid emitter release array removal (`phys_fn_003493`)

IDA decompilation at RVA `0x00085950` shows `Fluid::releaseEmitter`: it locates the internal emitter, calls the backend destroy entrypoint with the manager, fluid, and backend emitter, then swap-removes that index from both the internal and public emitter arrays. The empty-array and missing-emitter paths report `NXE_INVALID_PARAMETER`. The candidate implementation is `nxFluidReleaseEmitterInternal` in `Physics/src/Scene.cpp`, mapped to `0x10069100` in `Scene.obj`.

The registered `NxPhysicsSimulationTests` fixture creates a fourth emitter and releases the first of the original three. The new row-specific marker checks count 3, the moved last entry in both arrays, and one backend destroy callback. A mutation sourcing the internal replacement from `internalCount - 2` is detected: the internal array check fails while the public array remains correct (`internal-array=0`, `public-array=1`, `mismatches=1`) and the corrupted run later exits with `0xC0000005` (`stdout_delta=88`). The restored oracle and candidate both exit 0 and match exactly. Fresh Phase 5, 6, and 7 gates pass at 2,879/2,879, 1,739/1,739, and 3,018/3,018 assertions. The new marker appears on both sides in all three runs.

Raw transcripts and the mutation patch are in `evidence/raw/phase7-fluid-emitter-release-003493-20261017/`.
