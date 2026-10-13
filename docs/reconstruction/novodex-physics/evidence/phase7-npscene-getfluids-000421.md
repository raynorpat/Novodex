# `NpScene::getFluids` row 000421

IDA decompilation of `phys_fn_000421` at RVA `0xdac0` takes the scene read lock
from `this + 0x10`, reads the optional fluid manager at `Scene + 0x61c`, and
returns its public fluid-array pointer at manager offset `+0x14` (or null when
the manager is absent). The candidate now holds the same read lock around that
read. The candidate map records `?getFluids@NpScene@@UAEPAPAVNxFluid@@XZ` in
`NpScene.obj`.

The registered Phase 7 `NxPhysicsSimulationTests` fixture verifies the enabled
manager's public array against the internal fluid object and checks the empty
manager path. Replacing the getter with a null return makes the enabled fluid
lifecycle fail (`candidate_exit=1`); the staged differential measures
`stdout_delta=53`. Restoring the implementation produces an exact control
(`stdout_delta=0`, `stderr_exact=True`).

The full Win32 Release Phase 7 gate passes at
`coverage_assertions_evaluated=1722 floor=1722` (`phase_gate=7 status=pass`).
