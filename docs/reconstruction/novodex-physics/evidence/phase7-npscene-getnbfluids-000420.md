# `NpScene::getNbFluids` row 000420

IDA decompilation of `phys_fn_000420` at RVA `0xda80` takes the scene read lock
from `this + 0x10`, reads the optional fluid manager at `Scene + 0x61c`, and
returns the four-byte element count between manager offsets `+4` and `+8` (or
zero when no manager exists). The candidate now holds the same read lock around
the manager/count read. The candidate map records
`?getNbFluids@NpScene@@UBEIXZ` in `NpScene.obj`.

The registered Phase 7 `NxPhysicsSimulationTests` fixture exercises both the
empty/unsupported manager and the enabled manager with one live fluid. Replacing
the count with zero makes enabled fluid creation fail (`candidate_exit=1`); the
staged differential measures `stdout_delta=53`. Restoring the implementation
produces an exact control (`stdout_delta=0`, `stderr_exact=True`).

The full Win32 Release Phase 7 gate passes at
`coverage_assertions_evaluated=1722 floor=1722` (`phase_gate=7 status=pass`).
