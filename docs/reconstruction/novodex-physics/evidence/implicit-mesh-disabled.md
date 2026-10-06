# Disabled implicit-mesh scene API

The pinned SDK marks scene implicit meshes unavailable. The four exported
operations report `NXE_DB_WARNING` (206) from `NpScene.cpp`: creation returns
null, release ignores its reference, the count returns zero, and enumeration
returns null.

`NxPhysicsSimulationTests` exercises each method on the same scene used for the
disabled-fluid API checks. Release receives a reference to a local marker byte;
neither pinned nor candidate code reads it. The oracle's exact results are:

- create: line 641, `NxScene::createImplicitMesh(): Feature not available!`,
  null result
- release: line 647,
  `NxScene::releaseImplicitMesh(): Feature not available!`
- count: line 653,
  `NxScene::getNbImplicitMeshes(): Feature not available!`, zero result
- list: line 659, `NxScene::getImplicitMeshes(): Feature not available!`,
  null result

All warnings use `\Epic\Novodex\SDKs\Physics\src\NpScene.cpp`. Before the
wrapper reconstruction, the candidate omitted all four warnings (eight
coverage lines differed across the oracle/candidate pair). Afterward, the
staged differential passes with both processes exiting 0,
`stdout_delta=0`, and `stderr_exact=True` in
`build/FluidGate/implicit-mesh-green.log` (2026-10-05).

The four exact results are pinned in the Phase 7 registry. This closes only the
disabled public implicit-mesh entries; it does not implement an enabled
implicit-mesh backend or other fluid-manager behavior.
