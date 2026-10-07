# DemoGame `PhysTest` smoke on current main — 2026-10-07

The current local-main source revision (`0fbbdb81`) was freshly configured and
built as Win32 Release with Visual Studio 18 2026. The build produced
`NxPhysics.dll`, `NxPhysicsSimulationTests.exe`, and the Viewer.

The standalone `NxPhysicsSimulationTests` staged-pair differential is exact:
oracle and candidate both exit 0, `stdout_delta=0`, and stderr is byte-exact.
The corpus includes the 1,000-step stack, force and kinematic updates,
sleep/wake, contacts, jointed motion, all three broad-phase selectors, and
repeated SDK/scene lifecycle cases. Full runner output is retained in the
ignored build artifact `build/simulation-mainline-final.log`.

The all-Viewer CTest selection passed all 48 entries in 305.90 seconds. It
includes all 39 checked-in Viewer scenes, setup and sound checks, and the two
focused physics tests: 43 passed and five pinned-oracle asset failures skipped
on their existing exact diagnostic signatures (`CowPile`, `PMapTest8`,
`PMapTest10`, `PMapTest12`, and `TruckDemo`). No test failed.

For the Unreal smoke, `Binaries` and `Engine` were copied into separate
oracle/candidate roots under
`build/unreal-consumer-smoke-main-0fbbdb81`; `DemoGame/Config` and `Script`
were copied, and `DemoGame/Content` was junctioned to the installed asset tree.
Each root had its own `DemoGame/Logs`. The installed DLLs were left in place.
IDA's locked `NxPhysics.dll` database sidecars were not copied; every other
runtime binary file was present in both staged roots.

Both roots ran:

```text
DemoGame.exe PhysTest -BENCHMARK -SECONDS=2 -WINDOWED -NOSOUND -ABSLOG=<staged DemoGame/Logs/Launch.log>
```

The oracle exited 0 in 17.482 seconds and the candidate exited 0 in 15.494
seconds. Both logs reached `Browse: PhysTest`, `LoadMap: PhysTest`, `Game engine
initialized`, `Initializing Engine Completed`, `Object subsystem successfully
closed`, and `Exiting.` Module polling verified that each process loaded both
libraries from its own staged `Binaries` directory.

| Pair | NxPhysics SHA-256 | NxFoundation SHA-256 |
|---|---|---|
| Oracle | `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` | `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990` |
| Candidate | `27ae29d32da273521e96a203f3f56ccf0283ddf8da53e271eff94e23668a97b1` | `f2e9688e59e96ca3fabb11703f971c6333e307efa1cfafd28dce04a04887324c` |

This reaches the roadmap's initial T1 standalone and T2 Unreal smoke checkpoints
on the current source. It covers one map and does not satisfy the broader
Unreal interaction matrix or full-DLL acceptance. Public Physics headers were
not changed.
