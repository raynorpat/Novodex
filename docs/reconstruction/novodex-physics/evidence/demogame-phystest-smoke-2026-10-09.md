# DemoGame `PhysTest` smoke — 2026-10-09

After the standalone simulation and all-scenes Viewer checks passed, ran the
same two-second `PhysTest` benchmark against fresh oracle and candidate staging
roots. Both roots used the same installed DemoGame executable, read-only Engine,
Development, and Content junctions, and private Physics/Foundation DLL copies.
No installed binary was replaced.

Staging root: `D:\github\Novodex\build\DemoGameSmoke-8ccab1c9`.

| Input | SHA-256 |
|---|---|
| `DemoGame.exe` in both roots | `ec2a9a115925479cd316bcd4d5d2edab2d462f832f2f662d94f5a100e614a6bc` |
| Oracle `NxPhysics.dll` | `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` |
| Oracle `NxFoundation.dll` | `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990` |
| Candidate `NxPhysics.dll` | `e4f5822a0109c43c20e7266f699568f724463269b33fa14a7520773b1c6c6da7` |
| Candidate `NxFoundation.dll` | `a72f3f6f361c0f9aa628461d05cbb5aea4ca819672460212e800d67ce8e1b9dc` |

Each root ran from its own staged `Binaries` directory:

```text
DemoGame.exe PhysTest -BENCHMARK -SECONDS=2 -WINDOWED -NOSOUND -ABSLOG=<staged DemoGame/Logs/PhysTest.log>
```

Both processes exited 0. The oracle run took 1.85 s and the candidate run took
1.42 s. Module polling observed each process loading both DLLs from its own
staged `Binaries` directory. Both logs reached `Browse: PhysTest`,
`LoadMap: PhysTest`, `Bringing Level PhysTest.MyLevel up for play`,
`Initializing Engine Completed`, and `Exiting.` They selected the same D3D
adapter (`AMD Radeon AI PRO R9700`) and emitted the same four
`setCMassOffsetLocalPosition` static-actor diagnostics.

Full logs:

- `D:\github\Novodex\build\DemoGameSmoke-8ccab1c9\oracle\DemoGame\Logs\PhysTest.log`
- `D:\github\Novodex\build\DemoGameSmoke-8ccab1c9\candidate\DemoGame\Logs\PhysTest.log`

The installed Physics and Foundation hashes remained at their pinned oracle
values after the run, and no DemoGame process remained. This confirms the
merged candidate pair loads and shuts down through the DemoGame `PhysTest`
map. It does not capture an asserted per-step body state, so the standalone
simulation differential remains the source of numeric step evidence and the
broader Unreal interaction matrix is still open.
