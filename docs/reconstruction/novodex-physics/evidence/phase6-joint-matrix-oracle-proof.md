# NxPhysicsJointTests oracle-only matrix proof

The Phase 6 `NxPhysicsJointTests` runner exercises 122 descriptor cases across
all ten joint families, including D6, fixed, and pulley cases, then prints the
resulting object state, saved descriptors, scene enumeration and cleanup
observations. The retained proof pins 284 explicit input lines (including the
D6 and pulley-specific descriptor fields and six transformed-fixture inputs)
and 2,873 oracle-output lines. It filters only the pair-directory, module
count and loaded-module path records, whose values are machine-dependent; the
oracle and source hashes remain pinned separately.

The canonical input SHA-256 is
`a35c701865c1044c912493ee63ffc759ceb70457ca779e086fd300b1da5f953e`; the
oracle-output SHA-256 is
`c23e3cb084a4cdbf5ed10eabc9e459cc93a4b8df7428a08e73d05a645a6b57e0`. The
case-family counts and self-hashed proof are recorded in
`oracle-only-baselines.json` and
`oracle-only-proofs/phase6-joint-matrix.json`.

Validation: the real oracle transcript passed the proof verifier. The fresh
Phase 6 gate is the integration check for the runner's machine-readable proof.
Public headers were not changed.
