# Phase 5 — `DynamicBody::markIslandDirty` (`phys_fn_000748`)

The registered `NxPhysicsBodyCreationTests` now calls the row directly on a minimal self-root record. It covers both branches: a non-null island sets the dirty bit (`1 -> 3`), while a null island preserves the existing flags (`5 -> 5`). The oracle is called at RVA `0x16f80`; the candidate address comes from its generated linker map.

Mutation check: changing the candidate update from `+0x1e4 |= 2` to `+0x1e4 |= 0` is detected. The oracle reports `bodycreate mark_island_dirty=3.5`; the mutant reports `1.5` and exits 1. After restoring the source and rebuilding, oracle and candidate match exactly (`stdout_delta=0`, `stderr_exact=True`).

This closes `phys_fn_000748` as an oracle differential falsification. Phase 5 moves to 125 closed / 80 deferred functions. Coverage advances to 2,310 assertions. Public headers are unchanged.
