# Box hull shared-hook accessor `phys_fn_000985`

`BoxHullFacade::sharedHook` (`NxPhysics.dll+0x00021a10`, BOX hull slot 0)
returns a stable pointer to a zero-initialized three-word block. The
`NxPhysicsObjectLayoutTests` pinned-oracle differential drives two oracle calls
and two candidate calls and checks pointer stability and all three words.

The clean baseline at commit `15e4f7d0` reported oracle
`stable=1 words=00000000.00000000.00000000`, candidate `ok=1 stable=1`, and
`layout candidate mismatches=0` (`build/phase5-box-sharedhook-baseline.log`).

Mutation: change the candidate's `return shared` to `return nullptr` in
`Physics/src/ObjectModel.cpp`, rebuild `NxPhysicsObjectLayoutTests`, and rerun
the pinned-oracle differential. The candidate reports `ok=0 stable=0`; the
oracle remains stable and zero-filled. The harness reports
`layout candidate mismatches=1` and exits 1 (`build/phase5-box-sharedhook-mutant.log`).

Restoration: restore `return shared`, rebuild, and rerun. The candidate returns
to `ok=1 stable=1`, `layout candidate mismatches=0`, and
`layout result=differential-pass` (`build/phase5-box-sharedhook-restored.log`).
The row is included in the Phase 5 `NxPhysicsObjectLayoutTests` oracle
differential and its pinned coverage line.

Reproduction commands from the repository root:

```powershell
cmake --build build --config Release --target NxPhysicsObjectLayoutTests
& .\build\Release\NxPhysicsObjectLayoutTests.exe 'D:\FlamingEnt__\Unreal_3\Binaries' '4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c'
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 5 -RepoRoot $PWD -BuildRoot (Join-Path $PWD 'build') -PairsRoot (Join-Path $PWD 'build/pairs-raycast-phase5')
```
