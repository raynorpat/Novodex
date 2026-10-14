# Scene timing copy-out `phys_fn_000542`

`phys_fn_000542` is the internal timing getter at RVA `0x10720`: it copies the timestep, maximum iteration count, and timestep method from Scene offsets `+0x52c`, `+0x530`, and `+0x534` into three caller outputs. The implementation is `NxSceneInternal::getTiming` in `Physics/src/Scene.cpp`; the existing simulation fixture observes the initial and changed triplets through the public API.

A throwaway mutation suppressed all three output copies. The registered `NxPhysicsSimulationTests` staged differential caught it (`stdout_delta=4`, both processes exit zero, exact stderr). The restored control is exact (`stdout_delta=0`, both exits zero, exact stderr). Fresh Phase 7 passed at `1,770/1,770` coverage assertions, including the immutable public-header check. Raw logs are in [`evidence/raw/phase7-scene-timing-helper-000542-20261013/`](raw/phase7-scene-timing-helper-000542-20261013/).
