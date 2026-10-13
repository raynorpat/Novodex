# `NpScene::getActors` row 000418

IDA decompilation of `phys_fn_000418` at RVA `0xda50` shows a read-lock enter
using the link at `this + 0x10`, a load of the contiguous actor-array pointer
from `Scene + 0x55c` through the internal Scene pointer at `this + 0x24`, and a
read-lock leave before returning the pointer. `NpScene::getActors` now follows
that sequence. The candidate map records `?getActors@NpScene@@UAEPAPAVNxActor@@XZ`
in `NpScene.obj`.

The registered Phase 7 `NxPhysicsSceneBoundsPlanesTests` verifies the returned
array across six bounds actors, the ground plane, and the combined seven-actor
scene. Replacing the method with `return 0` makes the candidate fail with
`candidate_exit=1`; the staged differential measures `stdout_delta=5`. The
restored control is exact (`stdout_delta=0`, `stderr_exact=True`).

The full Win32 Release Phase 7 gate passes at
`coverage_assertions_evaluated=1722 floor=1722` (`phase_gate=7 status=pass`).
