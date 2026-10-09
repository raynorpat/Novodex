# Phase 7 registered-gate refresh — 2026-10-09

Ran the registered Phase 7 gate against the current worktree and the pinned
oracle:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 `
  -Phase 7 -RepoRoot "$PWD" -BuildRoot "$PWD/build" `
  -OracleRoot 'D:\FlamingEnt__\Unreal_3' `
  -PairsRoot 'D:\FlamingEnt__\novodex-analysis\pairs\phase7-current-worktree-island-20261009'
```

The oracle `NxPhysics.dll` SHA-256 is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`; the
candidate staged from this worktree is
`bda129abbfc38eabc562cfb77c8e46b7c7dc198040f6847c70d1d7b06bb99ba9`.

All 14 registered differentials passed with both processes exiting zero,
`stdout_delta=0`, and exact stderr:

- `NxPhysicsJointStagedPairTests`
- `NxPhysicsJointAllocatorTests`
- `NxPhysicsJointSlotTests`
- `NxPhysicsSceneConstructorTests`
- `NxPhysicsSceneBoundsPlanesTests`
- `NxPhysicsControllerSweepFaceTests`
- `NxPhysicsSceneRaycastTests`
- `NxPhysicsSceneVisualizeTests`
- `NxPhysicsSimulationTests`
- `NxPhysicsPairFlagTests`
- `NxPhysicsMeshSimulationTests`
- `NxPhysicsTriggerSimulationTests`
- `NxPhysicsEffectorTests`
- `NxPhysicsCoreDumpTests`

The gate evaluated `1,440/1,440` registered coverage assertions and reported
`phase_gate=7 status=pass`. The simulation differential also reconfirmed the
oracle behavior for unsupported fluids: `createFluid()` returns null with the
feature-unavailable error. This refresh does not close Phase 7: `program.json`
still marks it pending with 530 function rows and 502 data rows remaining. The
complete command transcript is `build/phase7-refresh.log`.
