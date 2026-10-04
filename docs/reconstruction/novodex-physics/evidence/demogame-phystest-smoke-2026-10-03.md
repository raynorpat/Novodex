# DemoGame PhysTest consumer smoke — 2026-10-03

This is the first clean staged consumer run for the current CMake candidate. The earlier staged comparison used a stale September 20 `DemoGame.exe` (SHA-256 `75b1ed01e7a9cd1984ab20b70e81ca7b130868e9504f9d9958d94d04091e886b`) and failed in URL parsing with `Bad expr token 46` on both pairs. The installed engine now has the October 1 executable; refreshing only the isolated smoke roots with that executable removed the baseline launch failure. The installed engine DLLs were not replaced.

## Inputs

- Engine executable: `D:\FlamingEnt__\Unreal_3\Binaries\DemoGame.exe`, SHA-256 `ec2a9a115925479cd316bcd4d5d2edab2d462f832f2f662d94f5a100e614a6bc`, UE 470, compiled October 1, 2026, revision `0b9921b7`.
- Map: `PhysTest`, from `DemoGame\Content\Maps\Testmaps\PhysTest.war`.
- Command, run from each staged `Binaries` directory: `DemoGame.exe PhysTest -BENCHMARK -SECONDS=2 -WINDOWED -NOSOUND`.
- Oracle pair: NxPhysics `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`; NxFoundation `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990`.
- Candidate pair: CMake-built NxPhysics `294cb0d013af59b1b59a02a7fa493c28c872a2fa69a5e3853c23d043a072a633`; CMake-built NxFoundation `2b7214813d70d9d043c774e5ee53e66ecda9afd4f45f695f929afb7f38c60e99`.

## Results

| Pair | Process exit | Elapsed | Loaded modules |
|---|---:|---:|---|
| Oracle | 0 | 2.5 s | `oracle\Binaries\NxPhysics.dll`, `oracle\Binaries\NxFoundation.dll` |
| Candidate | 0 | 1.9 s | `candidate\Binaries\NxPhysics.dll`, `candidate\Binaries\NxFoundation.dll` |

Both logs report `Unreal engine initialized`, `Browse: PhysTest`, `LoadMap: PhysTest`, `Game engine initialized`, `Initializing Engine Completed`, `Object subsystem successfully closed`, and `Exiting.` Neither reports the earlier candidate-only body/actor initialization failures or `Scene.cpp:552` failure. Both report the same four `Actor::setCMassOffsetLocalPosition: Actor must be (non-kinematic) dynamic!` diagnostics while bringing the level up, so these are present in the pinned baseline and are not evidence of a candidate-only divergence.

The complete logs and pair staging are retained under the ignored worktree build directory `build\unreal-consumer-smoke\{oracle,candidate}`. This validates one two-second DemoGame map load/simulation/unload smoke and confirms module provenance; it does not close the broader M5 map/transition/interactions matrix or the full-DLL acceptance criteria.
