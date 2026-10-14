# Scene timing store `phys_fn_000540`

`phys_fn_000540` is the internal three-word timing setter at RVA `0x106f0`: it stores the timestep, maximum iteration count, and timestep method at Scene offsets `+0x52c`, `+0x530`, and `+0x534`. The implementation is `NxSceneInternal::setTiming` in `Physics/src/Scene.cpp`; the public simulation fixture already exercises it through the timing API.

A throwaway mutation suppressed all three stores. The registered `NxPhysicsSimulationTests` staged differential rejected it (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=3990`; failed-candidate stderr was not exact). The restored control is exact (`stdout_delta=0`, both exits zero, exact stderr). The full Phase 7 gate passed at `1,770/1,770` registered assertions, including the immutable public-header check. Raw logs are in [`evidence/raw/phase7-scene-timing-helper-000540-20261013/`](raw/phase7-scene-timing-helper-000540-20261013/).
