# Phase 6 mutation evidence: `phys_fn_004145` — limit-plane iterator result

`phys_fn_004145` (`Joint::getNextLimitPlane`, RVA `0x0009a430`) reads the current plane against the joint limit point, advances the global iterator, and returns whether the point is in front. `NxPhysicsCoreDumpTests` explicitly iterates and records in-front results for joint limit planes.

The clean staged-pair differential passed with both exits 0, `stdout_delta=0`, and exact stderr. I temporarily inverted only the returned in-front boolean. The registered core-dump differential caught the mutation with both exits 0, `stdout_delta=2`, and exact stderr. Mutant candidate DLL SHA-256: `cf313cea979d8b2ba81d4c247587c6f3d8bcc0499ed1e39699bcb8d399ad16df`.

Restoring `Joint.cpp` and rebuilding with `--clean-first` returned the differential to exact output (`stdout_delta=0`, exact stderr). Restored candidate DLL SHA-256: `980a1189a65ddd20067490f9e515016eb9010a4659bfeecd2753dabe216fae6a`; pinned oracle: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The patch and raw mutant/restored transcripts are retained alongside this evidence. No public headers changed.

The full Phase 6 gate passed after this closure: all ten differential targets exact, with 1,554 coverage assertions run (minimum required: 1,267). Phase 6 remains pending with 102 closed and 331 deferred function rows.
