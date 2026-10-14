# Task M6.30 brief — Joint island dissolve (`phys_fn_000778`)

Base: `3e71c9eb`

An immediate-return mutation of `Row000778Fixture::row000778` is rejected by the registered simulation (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=4`, exact stderr). The joint staged-pair and allocator tests are exact with the mutant and do not independently exercise this path. Restoring `JointSupport.cpp` returns exact controls for all three targets.

Phase 7 moves to 318 closed / 242 deferred. No public headers changed. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-joint-dissolve-000778.md`.
