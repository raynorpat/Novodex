# CCD swept bounds (`phys_fn_000740`)

Date: 2026-10-14

Row: `phys_fn_000740` at RVA `0x00016c20`

Gate: Phase 7 `NxPhysicsCcdSimulationTests` staged-pair differential

The public two-step CCD collision scene did not distinguish a no-op of row 000740, so the registered CCD target now calls the function directly in both DLLs. It seeds a body record with a controlled velocity and center, a shape object whose vtable slot 10 returns a sphere, and a `dt` of 0.5. The oracle reports the expected validity flag and bounds: min `(-6,-6.5,-4)`, max `(9,9,10)`. The candidate map resolves `Row000740Fixture::row000740` from its emitted decorated symbol; the oracle uses pinned RVA `0x16c20`.

Replacing only the row with an immediate return is rejected (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=6`): the candidate leaves the bounds zero and the direct validity check fails. Restoring `BodyStep.cpp` byte-for-byte and rebuilding returns an exact control (`stdout_delta=0`, `stderr_exact=True`). The previous public CCD scene no-op control was also exact, documenting why the direct probe was necessary.

The direct result is registered in both Phase 6 and Phase 7; Phase 7 also includes the existing CCD scene's two assertions. The Phase 7 coverage floor is now 1,801. No public header or production source changed. Raw build, mutation, restored-control logs, hashes, and patch are under `evidence/raw/phase7-ccd-swept-bounds-000740-20261014/`.

After fast-forwarding the row to main at `f9d79202`, a fresh Win32 Release Phase 7 gate passed with `coverage_assertions_evaluated=1801 floor=1801` and `phase_gate=7 status=pass`. The compressed post-merge gate transcript is in the same raw evidence directory; its SHA-256 is `99994d8fb1aeaa39ba54385abeb72f419c54a15229c1229f77580b9211c6aeba`.
