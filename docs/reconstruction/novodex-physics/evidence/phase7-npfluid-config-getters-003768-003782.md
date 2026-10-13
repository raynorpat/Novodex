# `NxFluid` configuration getters (`phys_fn_003768`–`phys_fn_003782`)

The solver wrapper now dispatches the eight public configuration getters in
vtable slots 37–44. IDA shows that max-particles, rest-density, rest-particle
distance, particle mass, and simulation method forward to `FluidModel.dll`
while holding the scene read lock. Rest-particles-per-meter is computed as
`1.0f / FluidGetRestParticleDistance(...)`. Kernel-radius multiplier forwards
only when licensed feature mask `0x1` is present; otherwise it reports the
exact line-336 unavailable-feature warning. Product code 465 (`a-465-:`) enables
that branch in the test's isolated registry override. Collision method reports
the exact line-374 warning in this build.

The registered `NxPhysicsSimulationTests` fixture returns distinct values and
checks callback counts and both live backend pointers. Its public-api record is

```text
simulation fluid public-api configuration max=00001234 kernel=00000000.1.1 rest-per-meter=42480000 rest-density=447a0000 rest-distance=3ca3d70a particle-mass=3f2aaaab simulation=00000002 collision=00000000.1.1 callbacks=1.1.0.1.2.1.1
simulation fluid public-api configuration licensed-kernel=3fe00000 calls=1 errors=0 setup=1 fluid=1
```

The initial staged differential was red: the oracle exited 0 and the candidate
access-violated on the missing solver-vtable slots (`candidate_exit=-1073741819`,
`stdout_delta=58`). The restored differential passes exactly (`oracle_exit=0`,
`candidate_exit=0`, `stdout_delta=0`, exact stderr). Eight isolated mutations
were caught by the same registered test:

| Row | Mutation | Detection |
|---|---|---|
| `phys_fn_003768` | Return zero for max particles | `stdout_delta=52`, candidate exit 1 |
| `phys_fn_003770` | Use the wrong licensed feature bit, or omit the backend callback on the licensed path | Each mutation: `stdout_delta=52`, candidate exit 1 |
| `phys_fn_003772` | Return a wrong rest-particles-per-meter result | `stdout_delta=54`, candidate exit 1 |
| `phys_fn_003774` | Return zero for rest density | `stdout_delta=52`, candidate exit 1 |
| `phys_fn_003776` | Return zero for rest-particle distance | `stdout_delta=54`, candidate exit 1 |
| `phys_fn_003778` | Return zero for particle mass | `stdout_delta=52`, candidate exit 1 |
| `phys_fn_003780` | Return zero for simulation method | `stdout_delta=52`, candidate exit 1 |
| `phys_fn_003782` | Report collision warning on the wrong line | `stdout_delta=52`, candidate exit 1 |

The licensed path uses an isolated HKLM override backed by a unique HKCU key.
Oracle and restored candidate both return `1.75` (`0x3fe00000`) with one
callback and no diagnostic; the restored staged differential matches exactly.
The wrong-feature-bit control (mask `0x2` instead of `0x1`) and a no-forward
mutation each change the transcript by 52 bytes and fail the candidate.

Each restored control matches the oracle. The fresh Phase 5 gate passes at
2,685/2,685 required assertions, and Phase 7 passes at 1,718/1,718. Their
compressed full transcripts and the focused wrong-bit, no-forward, and
restored differential logs are in
[`raw/phase7-npfluid-config-license-20261013`](raw/phase7-npfluid-config-license-20261013/).
No public headers changed. The full DLL reconstruction and the remaining
`NxFluid` methods remain open.

Raw red, restored, and mutant transcripts are compressed in
[`raw/phase7-npfluid-config-getters-20261013`](raw/phase7-npfluid-config-getters-20261013/).
