# `phys_fn_003501` — fluid emitter creation

The oracle function at RVA `0x00085be0` implements `NxFluid::createEmitter`. IDA shows descriptor validation, a 20-emitter capacity check, allocation and construction of the internal emitter, backend initialization, and insertion into paired internal and public emitter arrays. The candidate helper is `nxFluidCreateEmitter` in `Physics/src/Scene.cpp`; the Release map places it at `0x10067940` in `Scene.obj`.

The registered `NxPhysicsSimulationTests` lifecycle fixture covers descriptor and allocation failures, the full capacity boundary, paired-array ordering, and callbacks. A 19-slot mutant (`0x50` to `0x4C`) fails: it creates only 16 additional emitters, stops at count 19, and the target exits 1 (`stdout_delta=89`). The restored candidate creates 17 to reach 20, rejects the next emitter, keeps both arrays and callbacks consistent, and matches the oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). The Phase 7 gate transcript records 3,017/3,017 assertions.

Raw captures and the mutation patch are in `evidence/raw/phase7-fluid-emitter-create-003501-20261017/`. The existing public `NxPhysics` headers were not changed.
