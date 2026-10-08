# Box shape raycast row `phys_fn_000949`

The planned next row was `phys_fn_000949` (`BoxShape::nxBoxRaycast`, BOX slot 5,
RVA `0x00020880`). Its registered public-SDK fixture is
`NxPhysicsSceneRaycastTests`, which creates real scenes and box actors and
drives the box raycast through the candidate DLL's public scene raycast path.
The clean baseline was exact: both processes exited 0, `stdout_delta=0`, and
stderr matched exactly. The restored candidate DLL SHA-256 was
`f2a4bc84331674fa4e4f4c0b17638bc9a76a653016038cf33d2c141dc44c7557`.

Mutation: reverse the row's hit-normal hint condition from `flags & 4` to
`(flags & 4) == 0` in `Physics/src/ObjectModel.cpp`, then rebuild `NxPhysics`
and `NxPhysicsSceneRaycastTests`. The registered differential rejected the
mutant with both processes exiting 0, exact stderr, and `stdout_delta=508`.
The mutant DLL SHA-256 was
`dcecd88ddac1c187aec1359cafdf07078fddb6e25449b8775590774bfb18fd3c`.

Restoration: restore the condition, rebuild both targets, and rerun the same
differential. It returned to both exits 0, `stdout_delta=0`, and exact stderr.
The restored DLL SHA-256 is
`f2a4bc84331674fa4e4f4c0b17638bc9a76a653016038cf33d2c141dc44c7557`.

The Phase 5 runner now includes `NxPhysicsSceneRaycastTests` and checks its
complete set of 249 pinned coverage lines, raising the Phase 5 coverage floor
from 2,311 to 2,560. The fresh full Phase 5 gate passed all 19 staged targets,
the static/oracle checks, and 2,560/2,560 required coverage assertions; see
`build/phase5-box-raycast-full-gate.log`.
This closes this row's observed behavior; it does not close its helper
`NxRayAABBIntersect2` or any remaining Phase 5 rows.

Reproduction commands from the repository root:

```powershell
cmake --build build --config Release --target NxPhysics NxPhysicsSceneRaycastTests
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Targets NxPhysicsSceneRaycastTests -RepoRoot $PWD -BuildRoot (Join-Path $PWD 'build') -PairsRoot (Join-Path $PWD 'build/pairs-scene-raycast')
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 5 -RepoRoot $PWD -BuildRoot (Join-Path $PWD 'build') -PairsRoot (Join-Path $PWD 'build/pairs-raycast-phase5')
```

Raw mutant and restored runner logs are retained in the ignored build tree as
`build/phase5-box-raycast-mutant.log` and
`build/phase5-box-raycast-restored.log`.
