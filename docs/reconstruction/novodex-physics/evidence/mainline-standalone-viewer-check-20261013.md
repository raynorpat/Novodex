# Mainline standalone and Viewer check — 2026-10-13

Source revision: `19b6f768`. The run used a fresh Win32 Release CMake build in `build/simulation-next-vs`. It did not read or modify the primary checkout's dirty files.

Build commands: `cmake -S . -B build/simulation-next-vs -G "Visual Studio 18 2026" -A Win32`; `cmake --build build/simulation-next-vs --config Release --target NxPhysicsSimulationTests --parallel 8`; `cmake --build build/simulation-next-vs --config Release --target NxPhysics --parallel 8`; `cmake --build build/simulation-next-vs --config Release --target Viewer ViewerPlatformTests ViewerSceneTests GraphicsImageTests GraphicsScriptTests GraphicsGeometryTests GraphicsSceneGraphTests GraphicsTerrainTests GraphicsLegacyTests GraphicsRenderingTests GraphicsOffscreenTests ViewerSoundTests --parallel 8`.

## Standalone simulation differential

- Command: `powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Targets NxPhysicsSimulationTests -RepoRoot . -BuildRoot build/simulation-next-vs -OracleRoot D:\FlamingEnt__\Unreal_3 -PairsRoot D:\FlamingEnt__\novodex-analysis\pairs\codex-simulation-20261013`
- Result: pass; oracle and candidate exited 0; `stdout_delta=0`; stderr exact.
- Oracle pair: `NxPhysics.dll` SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`; `NxFoundation.dll` SHA-256 `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990`.
- Candidate pair: `NxPhysics.dll` SHA-256 `9679b23aeb28cedaa203049e655e1096c62663f435dc850c3c3c9bf1a9af2b36`; `NxFoundation.dll` SHA-256 `8649e0e235864bca8f05c64bd89116762cfb322c7039f1807f265bb68b52e5da`.
- Raw transcript SHA-256: `a50310f5e372e1b3dcdedcf8c622f7f023fba3f9024895826cd30609722fbfc3`.
- Full staged-pair output (gzip): `phase6-standalone-simulation-20261013.log.gz`.

## Viewer all-scenes and physics checks

- Command: `ctest --test-dir build/simulation-next-vs -C Release -R '^Viewer' --output-on-failure`
- Result: 48/48 selected tests completed: 43 passed and five skipped on the registered pinned-oracle asset signatures. All 39 available scene entrypoints ran: 34 passed; four PMap scenes skipped because PMap creation fails in the pinned oracle (`CowPile`, `PMapTest8`, `PMapTest10`, `PMapTest12`); `TruckDemo` skipped because `Material #960.mat.ods` is absent. Both active Viewer physics checks passed (`ViewerPhysicsStep` and `ViewerPhysicsContact`).
- Full CTest transcript (gzip): `viewer-all-scenes-20261013.log.gz`; decompressed raw SHA-256: `2be0a7df407c5ea1b6600873be85b446f9d14e7566951f69d3360aabe6d64c0d`.

This is a consumer-test checkpoint, not full reconstruction closure. Phase 6/7 and the Phase 8 full-census audit remain open in the backlog.
