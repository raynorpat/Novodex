# Capsule save-to-state row `phys_fn_000991`

`CapsuleShape::nxCapsuleSaveState` (`NxPhysics.dll+0x00021b40`, CAPSULE slot
13) writes the capsule descriptor fields and tail-calls the base shape save
routine. The existing `NxPhysicsObjectLayoutTests` pinned-oracle differential
drives the row on a constructed capsule and compares the complete poisoned
0x58-byte record, including the returned success value.

The clean baseline reports oracle `saved=1 digest=bf079f3c`; the candidate
passes its capsule save check and the whole target reports
`layout candidate mismatches=0` (`build/phase5-box-sharedhook-full-gate.log`).

Mutation: replace the row's base-save return with `return false`, then rebuild
`NxPhysicsObjectLayoutTests`. The candidate reports `morerows candidate
okCap=0` and the pinned differential reports `layout candidate
mismatches=1`; the process exits 1 (`build/phase5-capsule-save-mutant.log`).

Restoration: restore the tail call, rebuild, and rerun. The candidate reports
`okCap=1`, the whole target returns to `mismatches=0` and
`layout result=differential-pass`, exit 0
(`build/phase5-capsule-save-restored.log`). This is an oracle-differential
proof for the row's return and full output-record path; it does not establish
unobserved non-default capsule-field values.

Reproduction commands from the repository root:

```powershell
cmake --build build --config Release --target NxPhysicsObjectLayoutTests
& .\build\Release\NxPhysicsObjectLayoutTests.exe 'D:\FlamingEnt__\Unreal_3\Binaries' '4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c'
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 5 -RepoRoot $PWD -BuildRoot (Join-Path $PWD 'build') -PairsRoot (Join-Path $PWD 'build/pairs-raycast-phase5')
```
