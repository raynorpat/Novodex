# Scene internal gravity setter row 000507

IDA decompilation of `phys_fn_000507` at RVA `0x101d0` stores the three words
of its `NxVec3` argument at `Scene + 0x520`, `+0x524`, and `+0x528`, then
returns the third word. `NxSceneInternal::setGravity` implements that store in
`Physics/src/Scene.cpp`; the candidate map records
`?setGravity@NxSceneInternal@@QAEXABVNxVec3@@@Z` in `Scene.obj`.

The registered Phase 7 `NxPhysicsSimulationTests` fixture compares the initial
and changed public gravity values and their simulation trajectories. Replacing
the helper body with a no-op keeps both processes alive but changes the staged
transcript by `stdout_delta=2`; the restored control is exact
(`stdout_delta=0`, `stderr_exact=True`).

The full Win32 Release Phase 7 gate passes at
`coverage_assertions_evaluated=1722 floor=1722` (`phase_gate=7 status=pass`).
