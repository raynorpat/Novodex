# Joint island dissolve (`phys_fn_000778`)

Date: 2026-10-14

Row: `phys_fn_000778` at RVA `0x000185f0`

Gate: Phase 7 `NxPhysicsSimulationTests` staged-pair differential. `NxPhysicsJointStagedPairTests` and `NxPhysicsJointAllocatorTests` were also checked to locate the detecting path.

This helper dissolves the root island when a joint is removed. It resolves the body root, walks each body in the island, queues every other joint link for later processing, clears each link word, and resets each body through row 000760. The later Scene path processes the collected joint array.

Replacing only `Row000778Fixture::row000778` with an immediate return is detected by `NxPhysicsSimulationTests`: both processes exit 0, stdout differs by 4 bytes, and stderr remains exact. The staged joint and allocator targets are transcript-exact under the same mutant and therefore do not independently detect the row.

Restoring `Physics/src/core/JointSupport.cpp` byte-for-byte returns exact controls for all three targets. The restored source SHA-256 is `5aa19f7651a86f9a2f370b4164fd7fbdaee73f089b2fefa8313b3367076a2259`; no public header changed. Mutation and restored builds, all three staged-pair transcripts, source hashes, and the patch are preserved under `evidence/raw/phase7-joint-dissolve-000778-20261014/`.

The full Phase 7 gate passes with `coverage_assertions_evaluated=1802 floor=1802` and `phase_gate=7 status=pass`. Its transcript is `evidence/raw/phase7-joint-dissolve-000778-20261014/phase7-gate.log.gz`.
