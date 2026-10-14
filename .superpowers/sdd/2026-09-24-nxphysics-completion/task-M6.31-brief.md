# Task M6.31 brief — Joint island continuation array push (`phys_fn_000780`)

Base: `7e08fdd1`

A direct `NxPhysicsSimulationTests` probe resolves the candidate method from its map and calls the oracle at pinned RVA 0x185f0. The seeded joint chain and spare-capacity array verify the continuation queues the non-removed joint, clears both links, and resets the body. Suppressing only the array push changes `queued=1` to `queued=0`; the mutant is rejected (`candidate_exit=1`, `stdout_delta=4000`) and the restored control is exact.

Phase 7 moves to 319 closed / 241 deferred. The direct assertion is registered in Phases 5, 6 and 7; their floors increase to 2,729, 1,654 and 1,803. No public headers changed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-joint-array-push-000780.md`.
