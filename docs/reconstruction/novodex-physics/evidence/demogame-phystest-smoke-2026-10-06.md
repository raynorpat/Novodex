# DemoGame PhysTest smoke on the current main build — 2026-10-06

The pinned oracle and the latest CMake-built candidate were staged into
separate `Binaries` directories under
`build/FluidGate/demo-game-smoke-20261006b`. DemoGame content is linked for
read-only asset access; each staged `DemoGame/Logs` directory is local. The
installed `D:\FlamingEnt__\Unreal_3\Binaries` DLLs were not changed.

Both pairs ran from their own staged `Binaries` directory with:

```text
DemoGame.exe PhysTest -BENCHMARK -SECONDS=2 -WINDOWED -NOSOUND -ABSLOG=<staged DemoGame/Logs/Launch.log>
```

| Pair | Exit | NxPhysics SHA-256 | NxFoundation SHA-256 |
|---|---:|---|---|
| Oracle | 0 | `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` | `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990` |
| Candidate | 0 | `c575fabd51cbbdb4795d8f2a886ed56658c6f0afb11d95ef1c7513d91ffd6860` | `9ca3e2c1035fef65374132363733d8359a6063cac215489abf9e5cdb9485a93e` |

Module polling confirmed each process loaded both DLLs from its own staged
`Binaries` directory. Both logs reached `Browse: PhysTest`, `LoadMap: PhysTest`,
`Game engine initialized`, `Initializing Engine Completed`,
`Object subsystem successfully closed`, and `Exiting.` Both also report the
same four `Actor::setCMassOffsetLocalPosition` diagnostics from `NpActor.cpp`
line 916; neither reports `Scene.cpp:552` or body-initialization failures.

Full logs and module snapshots are under
`build/FluidGate/demo-game-smoke-20261006b/{oracle,candidate}`. This reruns the
two-second startup/map-load/shutdown smoke against the current scheduler and
fluid-hook changes. It does not cover the broader Unreal map/interactions
matrix or establish full-DLL completion.
