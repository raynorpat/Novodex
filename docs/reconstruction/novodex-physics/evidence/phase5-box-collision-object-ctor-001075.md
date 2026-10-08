# Box-family collision-object constructor `phys_fn_001075`

`phys_fn_001075` (`NxPhysics.dll+0x00023580`) is the box-family constructor
variant of the 28-byte `CollisionObject`; it initializes the embedded member
and copies the owning shape pointer into both `+0x08` and `+0x18`. The
`NxPhysicsObjectLayoutTests` oracle differential reaches the box variant by
constructing a real `BoxShape` and checks both child back-pointers and the
embedded member.

The clean baseline reports `boxshape candidate ok=1 digest=ac5ed12f` and
`layout candidate mismatches=0` (`build/phase5-colobj-ctor-baseline.log`).

Mutation: replace the constructor's `mArgument18 = argument` store with null,
then rebuild `NxPhysicsObjectLayoutTests`. The box-shape check becomes
`boxshape candidate ok=0`; the overall pinned differential reports
`layout candidate mismatches=6` and exits 1
(`build/phase5-colobj-ctor-mutant.log`). Other concrete shape fixtures also
exercise this shared constructor sequence, but the box-specific assertion is
among the failures.

Restoration: restore the argument store, rebuild, and rerun. The box-shape
check returns to `ok=1`, the whole target returns to `mismatches=0`, and the
process exits 0 (`build/phase5-colobj-ctor-restored.log`). The generic
constructor entrypoint `phys_fn_001193` remains separately owned by Phase 3.

Reproduction commands from the repository root:

```powershell
cmake --build build --config Release --target NxPhysicsObjectLayoutTests
& .\build\Release\NxPhysicsObjectLayoutTests.exe 'D:\FlamingEnt__\Unreal_3\Binaries' '4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c'
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 5 -RepoRoot $PWD -BuildRoot (Join-Path $PWD 'build') -PairsRoot (Join-Path $PWD 'build/pairs-raycast-phase5')
```
