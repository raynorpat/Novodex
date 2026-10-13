# Phase 7 friction solver row (`phys_fn_000879`)

`NxActorPair::row000879` builds and solves the two tangential friction records for retained contact anchors. The registered `NxPhysicsSimulationTests` fixture creates a sphere sliding on a static plane with explicit nonzero static and dynamic friction, advances 60 fetched simulation steps, and records the final motion state.

A no-op mutation replaced the candidate row body with an immediate return. The contact callbacks still occurred, but the final state diverged: the oracle's x position/velocity were `0x3fbbb856` / `0x3fb101df`, while the mutant remained at `0x3ffffff4` / `0x40000000`. The staged differential reported `stdout_delta=132`. Restoring the implementation returned the full public simulation output to an exact match (`stdout_delta=0`, both processes exited zero).

The focused transcripts are in [raw evidence](raw/phase7-friction-row-000879/). Fresh Phase 5, Phase 6, and Phase 7 gates are recorded below after completion.

Fresh Phase 5 gate: pass at 2,700/2,700 assertions. See [phase5-gate.log](raw/phase7-friction-row-000879/phase5-gate.log).

Fresh Phase 6 gate: pass at 1,633/1,633 assertions. See [phase6-gate.log](raw/phase7-friction-row-000879/phase6-gate.log).

Fresh Phase 7 gate: pass at 1,742/1,742 assertions. See [phase7-gate.log](raw/phase7-friction-row-000879/phase7-gate.log). The mutated candidate DLL hash was 25a8dd0a9ba09ddb6a02dc2a5886640d96a7088218645a2107118ba57a98d8de; the restored DLL hash is f1cbf39933fdf0c5f49c615ea7b0d2647308f7a29c487addb6ff214a067406ea. The original and restored ContactPairManager.cpp hashes match exactly.
