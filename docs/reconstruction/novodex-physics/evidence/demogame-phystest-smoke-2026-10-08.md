# DemoGame `PhysTest` smoke — 2026-10-08

The Phase 6 and Phase 7 gates passed on the current mainline source before this consumer run. Phase 6 evaluated 1,067 coverage assertions; Phase 7 evaluated 1,388. The staged capsule/mesh differential and the other registered suite targets passed in those runs.

## Staging and inputs

The consumer copy is under `build/DemoGameSmoke-9f28a1cb`. The oracle and candidate have independent `Binaries` and local `DemoGame/Logs`; `Engine`, `Development`, and `DemoGame/Content` are junctions to the installed read-only trees. The installed DLLs were not replaced.

| Input | SHA-256 |
|---|---|
| `DemoGame.exe` in both roots | `ec2a9a115925479cd316bcd4d5d2edab2d462f832f2f662d94f5a100e614a6bc` |
| Oracle `NxPhysics.dll` | `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` |
| Oracle `NxFoundation.dll` | `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990` |
| Candidate `NxPhysics.dll` | `94893ce7925b34ad3afd5e0fd57a71e66d29eaa30e66c49d4579fee49d8a0b19` |
| Candidate `NxFoundation.dll` | `5d80df9a9051a0ed5cb0fc3c53cc3093cc534846226a52dd47c99df5721e48fe` |

Each root ran from its own staged `Binaries` directory:

```text
DemoGame.exe PhysTest -BENCHMARK -SECONDS=2 -WINDOWED -NOSOUND -ABSLOG=<staged DemoGame/Logs/PhysTest.log>
```

## Results

| Pair | Exit | Elapsed | Loaded Physics / Foundation modules |
|---|---:|---:|---|
| Oracle | 0 | 3.02 s | Both loaded from `oracle/Binaries` |
| Candidate | 0 | 2.24 s | Both loaded from `candidate/Binaries` |

Both logs reached `Browse: PhysTest`, `LoadMap: PhysTest`, `Bringing Level PhysTest.MyLevel up for play`, `Initializing Engine Completed`, and `Exiting.` Both emitted the same four `Actor::setCMassOffsetLocalPosition` diagnostics from `NpActor.cpp:916`; no candidate-only failure appeared. Full logs and module snapshots are retained under the ignored build directory named above.

This confirms the staged consumer can load the current DLL pair and load/unload the `PhysTest` map. It does not capture a physics callback or numeric body state, so actual simulated-activity validation and the broader Unreal interaction matrix remain open. After the run, the installed DLL hashes still matched the pinned oracle and there was no DemoGame process or DLL backup left behind.
