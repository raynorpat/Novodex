# Body CCD step update (`phys_fn_000770`)

Date: 2026-10-14

Row: `phys_fn_000770` at RVA `0x000183a0`

Gate: Phase 7 `NxPhysicsSimulationTests` staged-pair differential; additionally checked by `NxPhysicsCcdSimulationTests`.

The row initializes the time-of-impact state, snapshots rotation and quaternion state, updates body velocity and pose, refreshes world inertia, and—when continuous collision detection is enabled—computes swept bounds through row 000740.

Replacing only `Row000770Fixture::row000770` with an immediate return is rejected by both public targets. The simulation candidate exits 1 with a 7,134-byte transcript delta (`stderr_exact=False`). The CCD candidate also exits 1 (`stdout_delta=5`) because the response velocity falls outside the expected range, while the direct row 000740 bounds probe itself still reports the expected values.

Restoring `Physics/src/BodyStep.cpp` byte-for-byte yields exact controls for both targets (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). The restored source SHA-256 is `d0bc5db97a4bd87e02924283e54d166e3f8a1e18ff39e9ef6d8bc9bd65d1bf83`; no public header changed. Mutant and restored builds, both staged-pair transcripts, source hashes, and the patch are preserved under `evidence/raw/phase7-body-ccd-update-000770-20261014/`.

The full Phase 7 gate passes after recording this closure: `coverage_assertions_evaluated=1802 floor=1802`, `phase_gate=7 status=pass`. The transcript is `evidence/raw/phase7-body-ccd-update-000770-20261014/phase7-gate.log.gz`.
