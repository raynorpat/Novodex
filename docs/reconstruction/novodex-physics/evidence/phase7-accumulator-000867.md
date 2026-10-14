# Kind-selected accumulator row 000867 (`phys_fn_000867`)

Added a direct candidate/oracle fixture to the registered simulation staged-pair target. It calls the candidate map symbol `nxAccumulateByKind0867` and the oracle RVA `0x0001d260` for five descriptor kind/flag words across three seeded numerator/denominator pairs. The probe hashes the full 0x100-byte accumulator after each call, covering both kind-selected branches, kind masking, bit 6, positive and negative ratios, and zero numerator.

The restored oracle and candidate both produce digest `337db078` (`stdout_delta=0`, `stderr_exact=True`). An immediate-return mutation only in the candidate row changes the digest to `3695bd32`; the registered differential rejects it while both processes exit 0 (`stdout_delta=138`, exact stderr). Restoring `Physics/src/StepOnlyRows.cpp` byte-for-byte returns the exact control.

The previous `NxPhysicsObjectLayoutTests` cases called only the oracle row and a test-local formula. This direct probe now observes the built candidate DLL. The fixture adds no public-header changes. Mutation, source hashes, and focused transcripts are in `evidence/raw/phase7-accumulator-000867-20261014/`.


After commit `0958eb70` was fast-forwarded to local `main`, the clean-worktree Phase 5 and 6 gates passed at 2,732/2,732 and 1,657/1,657. Phase 7 passed at 1,979/1,979 on a fresh retry after the first oracle process access-violated; the successful simulation differential is exact. The main checkout also rejected its physical `PhysicsObjectLayoutTests.cpp` bytes against the pinned source hash (`40d2a639…` versus `54a97e79…`) although Git reports the file clean. That working-copy file was left untouched; the same commit passed from the clean managed worktree. All transcripts are in `evidence/raw/phase7-accumulator-000867-20261014/`.
