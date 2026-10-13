# Scene bounds-plane builder: `phys_fn_000501`

`phys_fn_000501` is the 393-byte bounds-plane builder at RVA `0x0000ff10`, implemented by `nxSceneBuildBoundsPlanes` in `Physics/src/Scene.cpp`. Its six faces are built in X-, X+, Y-, Y+, Z-, Z+ order. The already registered `NxPhysicsSceneBoundsPlanesTests` target checks asymmetric bounds, the default ground plane, and the combined ground-then-bounds order.

The baseline Win32 Release staged-pair differential passes exactly: both processes exit zero, the transcript delta is zero, and stderr is exact. To aim a mutation at this row, a throwaway source copy changed the six-iteration loop limit to five and rebuilt `NxPhysics.dll`. The pinned oracle still reported six boundary actors; the candidate reported five and failed the target's six-actor assertion. The differential rejected this mutation with `oracle_exit=0`, `candidate_exit=1`, `stdout_delta=7`, and non-identical stderr.

The source was restored byte-for-byte from its saved pre-mutation copy. A fresh Release rebuild reports six boundary faces, the default ground plane, and all seven combined planes in order; restored differential output is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). Restored candidate `NxPhysics.dll` SHA-256: `f7158490821ce1276aa9c61f879ca214cf9991a3bc16af0667343d1015675af3`. Pinned oracle SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

This closes only the bounds-plane builder row. The default-ground-plane helper and other scene descriptor behavior remain governed by their own census rows. Baseline, mutant, and restored differential transcripts are retained in [`evidence/raw/phase7-scene-bounds-planes-000501-20261012/`](raw/phase7-scene-bounds-planes-000501-20261012/).
