# Implicit-mesh float getters (`phys_fn_003876`, `phys_fn_003888`)

IDA identifies both entries as read-lock bracketed dispatchers. `getSurfaceRadius` takes the read lock through `this+0x10`, calls the internal record thunk at `0x8ad30` with `this+0x14`, unlocks, and returns the backend float. `getTriangleSize` follows the same sequence through thunk `0x8ae00`. The candidate implementations are in `Physics/src/ImplicitMesh.cpp`; candidate Release symbols resolve from `NxPhysics.map` to `ImplicitMesh.obj`.

The registered `NxPhysicsSimulationTests` probe creates a manager and fluid through the staged `FluidModel.dll` fixture, seeds the implicit-mesh owner/record and lock links, then directly calls each pinned oracle RVA and candidate map symbol. Three cases match the callback float bits (`0.04f` and `0.005f`) and preserve the x86 stack. The restored staged differential passes with `stdout_delta=0` and exact stderr.

Independent candidate-only mutations replacing either getter with a zero return are detected. The surface-radius mutation reports `mismatches=3`; the triangle-size mutation independently reports `mismatches=3`. Both staged differentials reject the candidate (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4119`).

Fresh Phase 5/6/7 gates pass at 2,890/2,890, 1,747/1,747 and 3,029/3,029 registered assertions. The tooling suite passes 836 tests and 743 subtests; all 80 public headers match their pinned hashes. Gate evidence is recorded in `evidence/raw/phase7-implicit-mesh-float-getters-003876-003888-20261017/`.
