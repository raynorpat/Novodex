# `NpScene::getNbActors` row 000416

IDA decompilation of `phys_fn_000416` at RVA `0xda10` loads the internal scene
pointer from `this + 0x24`, reads the actor-array begin/end pointers at
`Scene + 0x55c` and `Scene + 0x560`, and returns their pointer difference in
four-byte actor slots. The candidate implementation is
`NpScene::getNbActors` in `Physics/src/NpScene.cpp`; the MSVC map places
`?getNbActors@NpScene@@UBEIXZ` in `NpScene.obj`.

The registered Phase 7 `NxPhysicsSceneBoundsPlanesTests` differential exercises
counts of six boundary actors, one ground-plane actor, and seven combined
actors. Replacing the method with a no-op return of zero makes the candidate
report `actors=0` and exit 1; the staged oracle differential detects
`stdout_delta=7`. Restoring the implementation produces `oracle_exit=0`,
`candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True`.

The full Win32 Release Phase 7 gate passes at
`coverage_assertions_evaluated=1722 floor=1722` (`phase_gate=7 status=pass`).
