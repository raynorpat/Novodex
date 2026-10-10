# Phase 6 mutation evidence: `phys_fn_004137` — global anchor value return

The joint staged-pair harness previously compared the value-return helper to the same virtual slot, so a shared wrong value passed. `PhysicsJointTests.cpp` now records the actual anchor and axis bit patterns in the oracle differential, and the Phase 6 gate requires the family rows.

A temporary `+1.0f` X mutation in `Joint::getGlobalAnchorVal` changed the registered `NxPhysicsJointStagedPairTests` transcript (`stdout_delta=244`, both exits 0, exact stderr). Mutant DLL SHA-256: `374c62242612e073851ba541507a83a3ac5cce95d9368906595c6857a284d7a9`. After restoring the source and clean rebuilding, the staged differential matched exactly (`stdout_delta=0`, exact stderr); restored DLL SHA-256: `82a9a50859c4f8b59778ccda965692835901e5b9f8240f16a14682d6a56513aa`. Full mutant and restored transcripts are retained beside this file.

The full Phase 6 gate passes with all ten staged-pair targets exact and 1,564 coverage assertions run (minimum required: 1,267). Phase 6 remains pending with 104 closed and 329 deferred function rows.
