# Collision oracle-only proof (2026-10-14)

The checked-in `NxPhysicsCollisionTests` proof binds the pinned UE3 oracle DLL (`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`) and normalized fixture source (`tests/PhysicsCollisionTests.cpp`). It verifies the fixed pair/aimed seeds, 108 input-digest rows, 125 coverage labels, and 136 result rows. For result rows it retains only case identity, checks, and oracle result fields; candidate hashes, mismatch counters, and other candidate-only values are excluded.

The proof format has mutation tests: changing candidate-only fields leaves the proof identical; changing an oracle result or removing a coverage line fails. The fresh Phase 3 gate verifies `oracle_proof:NxPhysicsCollisionTests` and passes all 561/561 coverage assertions. The focused proof suite passes 24 tests, and the full reconstruction-tooling suite passes 816 tests.

The baseline and retained proof are `oracle-only-baselines.json` and `oracle-only-proofs/NxPhysicsCollisionTests.json`. The direct oracle transcript and Phase 3 gate log are gzip archived in `raw/phase3-collision-oracle-proof-20261014/`. This closes the M1 target-level oracle-only proof gap; it does not close any full-image DLL reconstruction rows.
