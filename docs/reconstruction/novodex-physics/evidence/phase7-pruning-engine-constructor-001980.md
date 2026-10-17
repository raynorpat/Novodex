# Pruning engine constructor `phys_fn_001980`

The Scene constructor calls `nxSceneMember4CA30` at `Scene+0x624`. This helper reproduces the oracle constructor's non-vtable stores, including the six empty-world AABB sentinels (`+0x04..+0x18`), cache and bounds fields, array header, and trailing `SdkContainer`. The candidate symbol is `?nxSceneMember4CA30@@YAXPAX@Z` at `0x1006bf50` in `Scene.obj`.

`NxPhysicsSceneRaycastTests` now checks the pruning engine's six bounds immediately after `createScene` and before shapes can update them. The pinned oracle and restored candidate both report `7f7fffff.7f7fffff.7f7fffff.ff7fffff.ff7fffff.ff7fffff`; the registered staged-pair differential passes with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and exact stderr.

Mutation proof: in a temporary Release build, changed the first minimum bound store in `nxSceneMember4CA30` from `FLT_MAX` to `1.0f`. The newly registered assertion reported `ctor_empty_bounds=0` on the candidate; the staged differential rejected it with `oracle_exit=0`, `candidate_exit=1`, and `stdout_delta=2813`. Restoring the constructor and rebuilding returned the differential to exact parity. Compressed raw mutant and restored transcripts are in `evidence/raw/phase7-pruning-engine-constructor-001980-20261017/`.
